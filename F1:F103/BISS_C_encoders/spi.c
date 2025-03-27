/*
 * This file is part of the encoders project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32f1.h>

#include "spi.h"
#include <string.h> // memcpy
#include "usb_dev.h"
#ifdef EBUG
#include "strfunc.h"
#endif

#define CHKIDX(idx) do{if(idx == 0 || idx > AMOUNT_OF_SPI) return;}while(0)
#define CHKIDXR(idx) do{if(idx == 0 || idx > AMOUNT_OF_SPI) return 0;}while(0)

spiStatus spi_status[AMOUNT_OF_SPI+1] = {0, SPI_NOTREADY, SPI_NOTREADY};
static volatile SPI_TypeDef* const SPIs[AMOUNT_OF_SPI+1] = {NULL, SPI1, SPI2};
static volatile DMA_Channel_TypeDef * const DMAs[AMOUNT_OF_SPI+1] = {NULL, DMA1_Channel2, DMA1_Channel4};
#define WAITX(x)  do{volatile uint32_t  wctr = 0; while((x) && (++wctr < 360000)) IWDG->KR = IWDG_REFRESH; if(wctr==360000){ DBG("timeout"); return 0;}}while(0)

static uint8_t encoderbuf[AMOUNT_OF_SPI][ENCODER_BUFSZ] = {0};
static uint8_t freshdata[AMOUNT_OF_SPI] = {0};

// init SPI to work RX-only with DMA
// SPI1 (PA5/PA6): DMA1_Channel2
// SPI2 (PB13/PB14): DMA1_Channel4
void spi_setup(uint8_t idx){
    CHKIDX(idx);
    volatile SPI_TypeDef *SPI = SPIs[idx];
    SPI->CR1 = 0; // clear EN
    SPI->CR2 = 0;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    volatile DMA_Channel_TypeDef *DMA = DMAs[idx];
    if(idx == 1){ // PA5/PA6; 72MHz
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
        RCC->APB2RSTR = RCC_APB2RSTR_SPI1RST;
        RCC->APB2RSTR = 0; // clear reset
        GPIOA->CRL = (GPIOA->CRL & ~(GPIO_CRL_CNF5 | GPIO_CRL_CNF6))
                     | CRL(5, CNF_AFPP|MODE_FAST) | CRL(6, CNF_FLINPUT);
        SPI->CR1 = SPI_CR1_BR_1 | SPI_CR1_BR_2; // Fpclk/128
        NVIC_EnableIRQ(DMA1_Channel2_IRQn); // enable Rx interrupt
    }else if(idx == 2){ // PB13/PB14; 36MHz
        RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
        RCC->APB1RSTR = RCC_APB1RSTR_SPI2RST;
        RCC->APB1RSTR = 0;
        GPIOB->CRH = (GPIOB->CRH & ~(GPIO_CRH_CNF13 | GPIO_CRH_CNF14))
                     | CRH(13, CNF_AFPP|MODE_FAST) | CRH(14, CNF_FLINPUT);
        SPI->CR1 = SPI_CR1_BR_0 | SPI_CR1_BR_2; // Fpclk/64
        NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    }else return; // err
    // Baudrate = 0b110 - fpclk/128
    SPI->CR1 |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_RXONLY;
    SPI->CR2 = SPI_CR2_RXDMAEN;
    DMA->CPAR = (uint32_t)&(SPI->DR);
    DMA->CCR = DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_TEIE; // mem inc, hw->mem, Rx complete and error interrupt
    spi_status[idx] = SPI_READY;
    DBG("SPI works");
}

void spi_onoff(uint8_t idx, uint8_t on){
    CHKIDX(idx);
    volatile SPI_TypeDef *SPI = SPIs[idx];
    if(on){
        DBGs(u2str(idx));
        DBG("turn on SPI");
        SPI->CR1 |= SPI_CR1_SPE;
        spi_status[idx] = SPI_BUSY;
    }else{
        SPI->CR1 &= ~SPI_CR1_SPE;
        spi_status[idx] = SPI_READY;
    }
}

// turn off given SPI channel and release GPIO
void spi_deinit(uint8_t idx){
    CHKIDX(idx);
    DBG("deinit SPI");
    volatile SPI_TypeDef *SPI = SPIs[idx];
    SPI->CR1 = 0;
    SPI->CR2 = 0;
    if(idx == 1){
        RCC->APB2ENR &= ~RCC_APB2ENR_SPI1EN;
        GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_CNF6);
    }else if(idx == 2){
        RCC->APB1ENR &= ~RCC_APB1ENR_SPI2EN;
        GPIOB->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_CNF14);
    }
    spi_status[idx] = SPI_NOTREADY;
}

static int spi_waitbsy(uint8_t idx){
    CHKIDXR(idx);
    if(SPIs[idx]->SR & SPI_SR_BSY){
        DBG("Busy - turn off");
        spi_onoff(idx, 0); // turn off SPI if it's busy
    }
    //DBGs(u2str(idx));
    //DBG("wait busy");
    //WAITX(SPIs[idx]->SR & SPI_SR_BSY);
    return 1;
}

// just copy last read encoder value into `buf`
// @return TRUE if got fresh data
int spi_read_enc(uint8_t encno, uint8_t buf[8]){
    if(encno > 1 || !freshdata[encno]) return FALSE;
    DBGs(u2str(encno)); DBG("Read encoder data");
    memcpy(buf, encoderbuf[encno], ENCODER_BUFSZ);
    freshdata[encno] = 0; // clear fresh status
    return TRUE;
}

// start encoder reading over DMA
// @return FALSE if SPI is busy
// here `encodernum` is 0 (SPI1) or 1 (SPI2), not 1/2 as SPI index!
int spi_start_enc(int encodernum){
    int spiidx = encodernum + 1;
    DBG("start enc");
    if(spiidx < 1 || spiidx > AMOUNT_OF_SPI) return FALSE;
    if(spi_status[spiidx] != SPI_READY) return FALSE;
    if(!spi_waitbsy(spiidx)) return FALSE;
    if(SPI1->CR1 & SPI_CR1_SPE) DBG("spi1 works!");
    if(SPI2->CR1 & SPI_CR1_SPE) DBG("spi2 works!");
    volatile DMA_Channel_TypeDef *DMA = DMAs[spiidx];
    DMA->CMAR = (uint32_t) encoderbuf[encodernum];
    DMA->CNDTR = ENCODER_BUFSZ;
    DBG("turn on spi");
    spi_onoff(spiidx, 1);
    DMA->CCR |= DMA_CCR_EN;
    return TRUE;
}

// SSI got fresh data
void dma1_channel2_isr(){
    // turn off DMA
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    if(DMA1->ISR & DMA_ISR_TEIF2){
        DMA1->IFCR = DMA_IFCR_CTEIF2;
    }
    if(DMA1->ISR & DMA_ISR_TCIF2){
        //uint32_t ctr = TIM2->CNT;
        DMA1->IFCR = DMA_IFCR_CTCIF2;
        freshdata[0] = 1;
        //encoderbuf[5] = (ctr >> 16) & 0xff;
        //encoderbuf[6] = (ctr >> 8 ) & 0xff;
        //encoderbuf[7] = (ctr >> 0 ) & 0xff;
    }
    spi_onoff(1, 0);
}

void dma1_channel4_isr(){
    // turn off DMA
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;
    if(DMA1->ISR & DMA_ISR_TEIF4){
        DMA1->IFCR = DMA_IFCR_CTEIF4;
    }
    if(DMA1->ISR & DMA_ISR_TCIF4){
        DMA1->IFCR = DMA_IFCR_CTCIF4;
        freshdata[1] = 1;
    }
    spi_onoff(2, 0);
}

