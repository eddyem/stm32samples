/*
 * This file is part of the F0testbrd project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "proto.h"
#include "spi.h"
#include "usb.h"

#include <string.h> // memcpy

// buffers for DMA rx/tx
static uint8_t inbuff[SPInumber][SPIBUFSZ], outbuf[SPInumber][SPIBUFSZ];
volatile uint8_t SPIoverfl[SPInumber] = {0, 0};
spiStatus SPI_status[SPInumber] = {SPI_NOTREADY, SPI_NOTREADY};
static DMA_Channel_TypeDef * const rxDMA[SPInumber] = {DMA1_Channel2, DMA1_Channel4};
static DMA_Channel_TypeDef * const txDMA[SPInumber] = {DMA1_Channel3, DMA1_Channel5};
static const uint32_t txDMAirqn[SPInumber] = {DMA1_Channel2_3_IRQn, DMA1_Channel4_5_6_7_IRQn};
static SPI_TypeDef * const SPI[SPInumber] = {SPI1, SPI2};

// setup SPI by data from arrays above,
// @param SPIidx - index in arrays
// @param master - == 0 for slave
static void spicommonsetup(uint8_t SPIidx, uint8_t master){
    if(SPIidx >= SPInumber) return;
    // Configure DMA SPI
    /* SPI_RX DMA config */
    /* (1) Peripheral address */
    /* (2) Memory address */
    /* (3) Data size */
    /* (4) Memory increment */
    /*     Peripheral to memory */
    /*     8-bit transfer */
    /*     Overflow IR */
    rxDMA[SPIidx]->CCR &= ~DMA_CCR_EN;
    rxDMA[SPIidx]->CPAR = (uint32_t)&(SPI[SPIidx]->DR); /* (1) */
    rxDMA[SPIidx]->CMAR = (uint32_t)inbuff[SPIidx]; /* (2) */
    rxDMA[SPIidx]->CNDTR = SPIBUFSZ; /* (3) */
    rxDMA[SPIidx]->CCR |= DMA_CCR_MINC | DMA_CCR_TEIE | DMA_CCR_EN; /* (4) */
    /* SPI_TX DMA config */
    /* (5) Peripheral address */
    /* (6) Memory address */
    /* (7) Memory increment */
    /*     Memory to peripheral*/
    /*     8-bit transfer */
    /*     Transfer complete IT */
    txDMA[SPIidx]->CCR &= ~DMA_CCR_EN;
    txDMA[SPIidx]->CPAR = (uint32_t)&(SPI[SPIidx]->DR); /* (5) */
    txDMA[SPIidx]->CMAR = (uint32_t)outbuf[SPIidx]; /* (6) */
    txDMA[SPIidx]->CCR |= DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_DIR; /* (7) */
    /* Configure IT */
    /* (8) Set priority */
    /* (9) Enable DMA */
    NVIC_SetPriority(txDMAirqn[SPIidx], 0);
    NVIC_EnableIRQ(txDMAirqn[SPIidx]);
    /* Configure SPI */
    /* (1) Master selection, BR: Fpclk/256 CPOL and CPHA at zero (rising first edge) */
    /* (1a) software slave management (SSI inactive) */
    /* (2) TX and RX with DMA, 8-bit Rx fifo */
    /* (3) Enable SPI */
    if(master) SPI[SPIidx]->CR1 = SPI_CR1_MSTR | SPI_CR1_BR | SPI_CR1_SSM | SPI_CR1_SSI; /* (1) */
    else  SPI[SPIidx]->CR1 = SPI_CR1_SSM; // (1a)
    SPI[SPIidx]->CR2 = SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN | SPI_CR2_FRXTH | SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0; /* (2) */
    SPI[SPIidx]->CR1 |= SPI_CR1_SPE; /* (3) */
    SPI_status[SPIidx] = SPI_READY;
    SPI_prep_receive(SPIidx);
}

// SPI1 (AF0): PB3 - SCK, PB4 - MISO, PB5 - MOSI; RxDMA - ch2, TxDMA - ch3
// SPI2 (AF0): PB13 - SCK, PB14 - MISO, PB15 - MOSI; RxDMA - ch4, TxDMA - ch5
void spi_setup(){
    // RCC->AHBENR |= RCC_AHBENR_GPIOBEN; // uncomment in common case
    /* (1) Select AF mode on pins */
    /* (2) AF0 for SPI1 signals */
    GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODER3 | GPIO_MODER_MODER4 | GPIO_MODER_MODER5|
                                     GPIO_MODER_MODER13 | GPIO_MODER_MODER14 | GPIO_MODER_MODER15)) |
                    GPIO_MODER_MODER3_AF  | GPIO_MODER_MODER4_AF  | GPIO_MODER_MODER5_AF |
                    GPIO_MODER_MODER13_AF | GPIO_MODER_MODER14_AF | GPIO_MODER_MODER15_AF; /* (1) */
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(GPIO_AFRL_AFRL3 | GPIO_AFRL_AFRL4 | GPIO_AFRL_AFRL5)); /* (2) */
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~(GPIO_AFRH_AFRH5 | GPIO_AFRH_AFRH6 | GPIO_AFRH_AFRH7)); /* (2) */
    // enable clocking
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    // SPI1 - master, SPI2 - slave
    spicommonsetup(0, 1);
    spicommonsetup(1, 0);
}

void spi_stop(){
    RCC->APB1ENR &= ~RCC_APB1ENR_SPI2EN;
    RCC->APB2ENR &= ~RCC_APB2ENR_SPI1EN;
}

/**
 * @brief SPI_transmit - transmit data over SPI DMA
 * @param N   - SPI number (0 - SPI1, 1 - SPI2)
 * @param buf - data to transmit
 * @param len - its length
 * @return 0 if all OK
 */
uint8_t SPI_transmit(uint8_t N, const uint8_t *buf, uint8_t len){
    if(!buf || !len || len > SPIBUFSZ || N >= SPInumber) return 1; // bad data format
    if(SPI_status[N] != SPI_READY) return 2; // spi not ready to transmit data
    int ctr = 0;
    while(SPI[N]->SR & SPI_SR_FTLVL && ++ctr < 99999); // wait for transmission buffer empty
    ctr = 0;
    while(SPI[N]->SR & SPI_SR_BSY && ++ctr < 99999); // wait while busy
    USND("SPI->SR="); USB_sendstr(uhex2str(SPI[N]->SR)); USND("\n");
    txDMA[N]->CCR &=~ DMA_CCR_EN;
    memcpy(outbuf[N], buf, len);
    txDMA[N]->CNDTR = len;
    //txDMA[N]->CMAR = (uint32_t)outbuf[N];
    SPI_prep_receive(N);
    SPI_status[N] = SPI_BUSY;
    txDMA[N]->CCR |= DMA_CCR_EN;
    return 0;
}

// prepare channel N to receive data, return 0 if all OK
uint8_t SPI_prep_receive(uint8_t N){
    if(N >= SPInumber) return 1;
    if(SPI_status[N] != SPI_READY) return 2; // still transmitting data
    SPIoverfl[N] = 0;
    rxDMA[N]->CCR &= ~DMA_CCR_EN;
    (void)SPI[N]->DR; // read DR and SR to clear OVR flag
    (void)SPI[N]->SR;
    rxDMA[N]->CNDTR = SPIBUFSZ;
    //rxDMA[N]->CMAR = (uint32_t)inbuff[N];
    rxDMA[N]->CCR |= DMA_CCR_EN;
    return 0;
}

/**
 * @brief SPI_getdata - get data received by DMA & reload receiver
 * @param N      - number of channel (0/1)
 * @param buf    - buffer for data (with length maxlen) or NULL
 * @param maxlen - (I) - amount of received bytes (or 0 if buffer is empty),
 *                 (O) - amount of real bytes amount in buffer (could be > maxlen if maxlen < SPIBUFSZ)
 * @return 0 if all OK or error code
 */
uint8_t SPI_getdata(uint8_t N, uint8_t *buf, uint8_t *maxlen){
    if(N >= SPInumber) return 1;
    if(SPI_status[N] != SPI_READY) return 2; // still transmitting data
    rxDMA[N]->CCR &= ~DMA_CCR_EN;
    SPIoverfl[N] = 0;
    uint8_t remain = rxDMA[N]->CNDTR;
    if(maxlen){
        if(buf && *maxlen) memcpy(buf, inbuff[N], *maxlen);
        *maxlen = SPIBUFSZ - remain; // bytes received
    }
    rxDMA[N]->CNDTR = SPIBUFSZ;
    rxDMA[N]->CCR |= DMA_CCR_EN;
    return 0;
}

// return 1 if given channel overflowed & clear overflow flag
// should be called BEFORE SPI_prep_receive() or SPI_getdata()
uint8_t SPI_isoverflow(uint8_t N){
    if(N >= SPInumber) return 1;
    register uint8_t o = SPIoverfl[N];
    SPIoverfl[N] = 0;
    return o;
}
