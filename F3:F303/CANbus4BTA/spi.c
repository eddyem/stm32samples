/*
 * This file is part of the canbus4bta project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "hardware.h"
#include "spi.h"

#include "usb.h"
#ifdef EBUG
#include "strfunc.h"
#endif

//#define SPIDR   *((volatile uint8_t*)&SPI2->DR)

spiStatus spi_status[AMOUNT_OF_SPI+1] = {0, SPI_NOTREADY, SPI_NOTREADY};
static volatile SPI_TypeDef* const SPIs[AMOUNT_OF_SPI+1] = {NULL, SPI1, SPI2};
#define WAITX(x)  do{volatile uint32_t  wctr = 0; while((x) && (++wctr < 360000)) IWDG->KR = IWDG_REFRESH; if(wctr==360000){ DBG("timeout"); return 0;}}while(0)

// SPI DMA Rx buffer (set by spi_write_dma call) for SPI2
//static uint8_t *rxbufptr = NULL;
//static uint32_t rxbuflen = 0;

// init SPI to work with (only SPI2) and without DMA (both)
// Channel 4 - SPI2 Rx
// Channel 5 - SPI2 Tx
void spi_setup(uint8_t idx){
    if(idx > AMOUNT_OF_SPI) return;
    volatile SPI_TypeDef *SPI = SPIs[idx];
    SPI->CR1 = 0; // clear EN
    SPI->CR2 = 0;
    if(idx == 1){ // PB3/PB4, without MOSI; SPI for SSI: AF5 @PB3, PB4
        GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(GPIO_AFRL_AFRL3 | GPIO_AFRL_AFRL4)) |
                        AFRf(5, 3) | AFRf(5, 4);
        GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODER3 | GPIO_MODER_MODER4)) |
                        MODER_AF(3) | MODER_AF(4);
        RCC->APB1RSTR = RCC_APB2RSTR_SPI1RST;
        RCC->APB1RSTR = 0; // clear reset
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
        SPI->CR1 = SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_RXONLY; // software slave management (without hardware NSS pin); RX only
    }else if(idx == 2){ // PB12..PB15
        GPIOB->AFR[1] = (GPIOB->AFR[1] & ~(GPIO_AFRH_AFRH4 | GPIO_AFRH_AFRH5 | GPIO_AFRH_AFRH6 | GPIO_AFRH_AFRH7)) |
                        AFRf(5, 12) | AFRf(5, 13) | AFRf(5, 14) | AFRf(5, 15);
        GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODER12 | GPIO_MODER_MODER13 | GPIO_MODER_MODER14 | GPIO_MODER_MODER15)) |
                       MODER_AF(12) | MODER_AF(13) | MODER_AF(14) | MODER_AF(15);
        RCC->APB1RSTR = RCC_APB1RSTR_SPI2RST; // reset SPI
        RCC->APB1RSTR = 0; // clear reset
        RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
        RCC->AHBENR |= RCC_AHBENR_DMA1EN;
        SPI->CR2 = SPI_CR2_SSOE; // hardware NSS management
        // setup SPI2 DMA
        //SPI->CR2 |= SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN;
        // Tx
        /*DMA1_Channel5->CPAR = (uint32_t)&(SPI2->DR); // hardware
        DMA1_Channel5->CCR = DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TEIE; // memory increment, mem->hw, error interrupt
        // Rx
        DMA1_Channel4->CPAR = (uint32_t)&(SPI2->DR);
        DMA1_Channel4->CCR = DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_TEIE; // mem inc, hw->mem, Rx complete and error interrupt
        NVIC_EnableIRQ(DMA1_Channel4_IRQn); // enable Rx interrupt
        NVIC_EnableIRQ(DMA1_Channel5_IRQn); // enable Tx interrupt
        */
    }else return; // err
    // Baudrate = 0b110 - fpclk/128
    SPI->CR1 |= SPI_CR1_MSTR | SPI_CR1_BR_2 | SPI_CR1_BR_1;
    // DS=8bit; RXNE generates after 8bit of data in FIFO;
    SPI->CR2 |= SPI_CR2_FRXTH | SPI_CR2_DS_2|SPI_CR2_DS_1|SPI_CR2_DS_0;
    spi_status[idx] = SPI_READY;
    SPI->CR1 |= SPI_CR1_SPE;
    DBG("SPI works");
}

int spi_waitbsy(uint8_t idx){
    if(idx > AMOUNT_OF_SPI) return 0;
    WAITX(SPIs[idx]->SR & SPI_SR_BSY);
    return 1;
}

/**
 * @brief spi_writeread - send data over SPI (change data array with received bytes)
 * @param data - data to write/read
 * @param n - length of data
 * @return 0 if failed
 */
int spi_writeread(uint8_t idx, uint8_t *data, uint32_t n){
    if(idx > AMOUNT_OF_SPI) return 0;
    if(spi_status[idx] != SPI_READY || !data || !n){
        DBG("not ready");
        return 0;
    }
    for(uint32_t x = 0; x < n; ++x){
        WAITX(!(SPIs[idx]->SR & SPI_SR_TXE));
        *((volatile uint8_t*)&SPIs[idx]->DR) = data[x];
        WAITX(!(SPIs[idx]->SR & SPI_SR_RXNE));
        data[x] = *((volatile uint8_t*)&SPIs[idx]->DR);
    }
    return 1;
}

/**
 * @brief spi_send_dma - send data over SPI2 through DMA (used both for writing and reading)
 * @param data - data to read
 * @param rxbuf - pointer to receiving buffer (at least n bytes), can be also `data` (if `data` isn't const)
 * @param n - length of data
 * @return 0 if failed
 * !!! `data` buffer can be changed only after SPI_READY flag!
 */
/*int spi_write_dma(const uint8_t *data, uint8_t *rxbuf, uint32_t n){
    if(spi_status[2] != SPI_READY) return 0;
    if(!spi_waitbsy(2)) return 0;
    rxbufptr = rxbuf;
    rxbuflen = n;
    // spi_setup(); - only so we can clear Rx FIFO!
    DMA1_Channel5->CMAR = (uint32_t) data;
    DMA1_Channel5->CNDTR = n;
    // check if user want to receive data
    if(rxbuf){
        DMA1_Channel4->CCR |= DMA_CCR_TCIE;
        DMA1_Channel5->CCR &= ~DMA_CCR_TCIE; // turn off Tx ready interrupt
        DMA1_Channel4->CMAR = (uint32_t) rxbuf;
        DMA1_Channel4->CNDTR = n;
        DMA1_Channel4->CCR |= DMA_CCR_EN; // turn on reception
    }else DMA1_Channel5->CCR |= DMA_CCR_TCIE; // interrupt by Tx ready - user don't want reception
    spi_status[2] = SPI_BUSY;
    DMA1_Channel5->CCR |= DMA_CCR_EN; // turn on transmission
    return 1;
}*/

/**
 * @brief spi_read - read SPI2 data
 * @param data - data to read
 * @param n - length of data
 * @return n
 */
/*
int spi_read(uint8_t idx, uint8_t *data, uint32_t n){
    if(idx > AMOUNT_OF_SPI) return 0;
    if(spi_status[idx] != SPI_READY){
        DBG("not ready");
        return 0;
    }
    if(!spi_waitbsy(idx)) return 0;
    // clear SPI Rx FIFO
    for(int i = 0; i < 4; ++i) (void) SPIs[idx]->DR;
    for(uint32_t x = 0; x < n; ++x){
        WAITX(!(SPIs[idx]->SR & SPI_SR_TXE));
        *((volatile uint8_t*)&SPIs[idx]->DR) = 0;
        WAITX(!(SPIs[idx]->SR & SPI_SR_RXNE));
        data[x] = *((volatile uint8_t*)&SPIs[idx]->DR);
        //USB_sendstr("rd got "); USB_sendstr(uhex2str(data[x]));
        newline();
    }
    return 1;
}*/

/**
 * @brief spi_read_dma - got buffer read by DMA
 * @param n (o) - length of rxbuffer
 * @return amount of bytes read
 */
/*uint8_t *spi_read_dma(uint32_t *n){
    if(spi_status[2] != SPI_READY || rxbuflen == 0) return NULL;
    if(n) *n = rxbuflen - DMA1_Channel4->CNDTR; // in case of error buffer would be underfull
    rxbuflen = 0; // prevent consequent readings
    return rxbufptr;
}*/

/*
// Rx ready interrupt
void dma1_channel4_isr(){
    spi_status[2] = SPI_READY; // ready independent on errors or Rx ready
    DMA1->IFCR = DMA_IFCR_CTCIF4 | DMA_IFCR_CTEIF4;
    // turn off DMA
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;
    DMA1_Channel5->CCR &= ~DMA_CCR_EN;
}

// Tx ready interrupt
void dma1_channel5_isr(){
    spi_status[2] = SPI_READY; // ready independent on errors or Tx ready
    DMA1->IFCR = DMA_IFCR_CTCIF5 | DMA_IFCR_CTEIF5;
    // turn off DMA
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;
    DMA1_Channel5->CCR &= ~DMA_CCR_EN;
}
*/
