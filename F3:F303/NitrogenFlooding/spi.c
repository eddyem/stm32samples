/*
 * This file is part of the nitrogen project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#define SPIDR   *((uint8_t*)&SPI2->DR)

spiStatus spi_status = SPI_NOTREADY;
volatile uint32_t wctr;
#define WAITX(x)  do{wctr = 0; while((x) && (++wctr < 360000)) IWDG->KR = IWDG_REFRESH; if(wctr==360000){ DBG("timeout"); return 0;}}while(0)

// SPI DMA Rx buffer (set by spi_write_dma call)
static uint8_t *rxbufptr = NULL;
static uint32_t rxbuflen = 0;

// init SPI2 to work with and without DMA
// ILI9341: SCL 0->1; CS=0; command - DC=0, data - DC=1; 1 dummy clock pulse before 24/32 bit data read
// Channel 4 - SPI2 Rx
// Channel 5 - SPI2 Tx
void spi_setup(){
    SPI2->CR1 = 0; // clear EN
    //RCC->APB1RSTR = RCC_APB1RSTR_SPI2RST; // reset SPI
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    // Baudrate = 0b011 - fpclk/16 = 2MHz; software slave management (without hardware NSS pin)
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_SSM | SPI_CR1_SSI;
    // 8bit; RXNE generates after 8bit of data in FIFO
    SPI2->CR2 = SPI_CR2_FRXTH | SPI_CR2_DS_2|SPI_CR2_DS_1|SPI_CR2_DS_0 | SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN;
    // setup SPI2 DMA
    // Tx
    DMA1_Channel5->CPAR = (uint32_t)&(SPI2->DR); // hardware
    DMA1_Channel5->CCR = DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TEIE; // memory increment, mem->hw, error interrupt
    // Rx
    DMA1_Channel4->CPAR = (uint32_t)&(SPI2->DR);
    DMA1_Channel4->CCR = DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_TEIE; // mem inc, hw->mem, Rx complete and error interrupt
    NVIC_EnableIRQ(DMA1_Channel4_IRQn); // enable Rx interrupt
    NVIC_EnableIRQ(DMA1_Channel5_IRQn); // enable Tx interrupt
    spi_status = SPI_READY;
    SPI2->CR1 |= SPI_CR1_SPE;
    DBG("SPI works");
}

int spi_waitbsy(){
    WAITX(SPI2->SR & SPI_SR_BSY);
    return 1;
}

/**
 * @brief spi_send - send data over SPI2 (change data array with received bytes)
 * @param data - data to read
 * @param n - length of data
 * @return 0 if failed
 */
int spi_write(const uint8_t *data, uint32_t n){
    if(spi_status != SPI_READY || !data || !n){
        DBG("not ready");
        return 0;
    }
    for(uint32_t x = 0; x < n; ++x){
        WAITX(!(SPI2->SR & SPI_SR_TXE));
        SPIDR = data[x];
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
int spi_write_dma(const uint8_t *data, uint8_t *rxbuf, uint32_t n){
    if(spi_status != SPI_READY) return 0;
    rxbufptr = rxbuf;
    rxbuflen = n;
    if(!spi_waitbsy()) return 0;
    // clear SPI Rx FIFO
    (void) SPI2->DR;
    while(SPI2->SR & SPI_SR_RXNE) (void) SPI2->DR;
    //DMA1_Channel4->CCR &= ~DMA_CCR_EN; // turn off to reconfigure
    //DMA1_Channel5->CCR &= ~DMA_CCR_EN;
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
    spi_status = SPI_BUSY;
    DMA1_Channel5->CCR |= DMA_CCR_EN; // turn on transmission
    return 1;
}

/**
 * @brief spi_read - read SPI2 data
 * @param data - data to read
 * @param n - length of data
 * @return n
 */
int spi_read(uint8_t *data, uint32_t n){
    if(spi_status != SPI_READY){
        DBG("not ready");
        return 0;
    }
    if(!spi_waitbsy()) return 0;
    // clear SPI Rx FIFO
    (void) SPI2->DR;
    while(SPI2->SR & SPI_SR_RXNE) (void) SPI2->DR;
    for(uint32_t x = 0; x < n; ++x){
        WAITX(!(SPI2->SR & SPI_SR_TXE));
        SPIDR = 0;
        WAITX(!(SPI2->SR & SPI_SR_RXNE));
        data[x] = SPIDR;
        USB_sendstr("rd got "); USB_sendstr(uhex2str(data[x]));
        newline();
    }
    return 1;
}

/**
 * @brief spi_read_dma - got buffer read by DMA
 * @param n (o) - length of rxbuffer
 * @return amount of bytes read
 */
uint8_t *spi_read_dma(uint32_t *n){
    if(spi_status != SPI_READY || rxbuflen == 0) return NULL;
    if(n) *n = rxbuflen - DMA1_Channel4->CNDTR; // in case of error buffer would be underfull
    rxbuflen = 0; // prevent consequent readings
    return rxbufptr;
}

// Rx ready interrupt
void dma1_channel4_isr(){
    spi_status = SPI_READY; // ready independent on errors or Rx ready
    DMA1->IFCR = DMA_IFCR_CTCIF4 | DMA_IFCR_CTEIF4;
    // turn off DMA
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;
    DMA1_Channel5->CCR &= ~DMA_CCR_EN;
}

// Tx ready interrupt
void dma1_channel5_isr(){
    spi_status = SPI_READY; // ready independent on errors or Tx ready
    DMA1->IFCR = DMA_IFCR_CTCIF5 | DMA_IFCR_CTEIF5;
    // turn off DMA
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;
    DMA1_Channel5->CCR &= ~DMA_CCR_EN;
}
