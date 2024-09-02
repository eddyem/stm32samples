/*
 * This file is part of the Stepper project.
 * Copyright 2020 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <string.h> // memcpy

// buffers for DMA rx/tx
static uint8_t inbuff[SPIBUFSZ], outbuf[SPIBUFSZ];
spiStatus SPI_status = SPI_NOTREADY;

//SPI: PA5 - SCK, PA6 -MISO, PA7 - MOSI, PC14 - ~CS
void spi_setup(){
    /* (1) Select AF mode on PA5, PA6, PA7 */
    /* (2) AF0 for SPI1 signals */
    GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER5 | GPIO_MODER_MODER6 | GPIO_MODER_MODER7)) |
                    (GPIO_MODER_MODER5_AF | GPIO_MODER_MODER6_AF | GPIO_MODER_MODER7_AF); /* (1) */
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(GPIO_AFRL_AFRL5 | GPIO_AFRL_AFRL6 | GPIO_AFRL_AFRL7)); /* (2) */
    // Configure DMA SPI
    /* Enable the peripheral clock DMA11 */
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    /* DMA1 Channel2 SPI1_RX config */
    /* (1) Peripheral address */
    /* (2) Memory address */
    /* (3) Data size */
    /* (4) Memory increment */
    /*     Peripheral to memory */
    /*     8-bit transfer */
    DMA1_Channel2->CPAR = (uint32_t)&(SPI1->DR); /* (1) */
    DMA1_Channel2->CMAR = (uint32_t)inbuff; /* (2) */
    DMA1_Channel2->CNDTR = SPIBUFSZ; /* (3) */
    DMA1_Channel2->CCR |= DMA_CCR_MINC | DMA_CCR_EN; /* (4) */
    /* DMA1 Channel3 SPI1_TX config */
    /* (5) Peripheral address */
    /* (6) Memory address */
    /* (7) Memory increment */
    /*     Memory to peripheral*/
    /*     8-bit transfer */
    /*     Transfer complete IT */
    DMA1_Channel3->CPAR = (uint32_t)&(SPI1->DR); /* (5) */
    DMA1_Channel3->CMAR = (uint32_t)outbuf; /* (6) */
    DMA1_Channel3->CCR |= DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_DIR; /* (7) */
    /* Configure IT */
    /* (8) Set priority for DMA1_Channel2_3_IRQn */
    /* (9) Enable DMA1_Channel2_3_IRQn */
    NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0); /* (8) */
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn); /* (9) */
    // configure SPI (transmit only)
    /* Enable the peripheral clock SPI1 */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    /* Configure SPI1 in master */
    /* (1) Master selection, BR: Fpclk/256 CPOL and CPHA at zero (rising first edge) */
    /* (2) TX and RX with DMA, slave select output enabled, 8-bit Rx fifo */
    /* (3) Enable SPI1 */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_BR; /* (1) */
    SPI1->CR2 = SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN | SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0; /* (2) */
    SPI1->CR1 |= SPI_CR1_SPE; /* (3) */
    SPI_status = SPI_READY;
}

void dma1_channel2_3_isr(){
    if(DMA1->ISR & DMA_ISR_TCIF3){
        DMA1->IFCR |= DMA_IFCR_CTCIF3;
        SPI_status = SPI_READY;
#ifdef EBUG
    SEND("rxCNDTR="); printu(DMA1_Channel2->CNDTR); SEND(", txCNDTR="); printu(DMA1_Channel3->CNDTR); newline(); sendbuf();
#endif
    }
}

/**
 * @brief SPI_transmit - transmit data over SPI DMA
 * @param buf - data to transmit
 * @param len - its length
 * @return 0 if all OK
 */
uint8_t SPI_transmit(const uint8_t *buf, uint8_t len){
    if(!buf || !len || len > SPIBUFSZ) return 1; // bad data format
    if(SPI_status != SPI_READY) return 2; // spi not ready to transmit data
    DMA1_Channel3->CCR &=~ DMA_CCR_EN;
    memcpy(outbuf, buf, len);
    DMA1_Channel3->CNDTR = len;
    SPI_status = SPI_BUSY;
#ifdef EBUG
    SEND("SPI tx "); printu(len); SEND(" bytes\n"); sendbuf();
#endif
    DMA1_Channel3->CCR |= DMA_CCR_EN;
    return 0;
}

/*
static uint8_t *SPI_receive(uint8_t *len){
    if(!len || SPI_status != SPI_READY) return NULL;
    *len =
}*/
