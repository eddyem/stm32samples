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

#include <string.h> // memcpy
#include "usart.h"
#include "spi.h"


// buffers for DMA rx/tx
static uint8_t inbuff[SPIBUFSZ], outbuff[SPIBUFSZ];
static uint8_t rxrdy = 0;

// SPI1 (AF0): PB3 - SCK, PB4 - MISO, PB5 - MOSI; RxDMA - ch2
void spi_setup(){
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN; // uncomment in common case
    /* (1) Select AF mode on pins */
    /* (2) AF0 for SPI1 signals */
    GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODER3 | GPIO_MODER_MODER4)) |
                    GPIO_MODER_MODER3_AF  | GPIO_MODER_MODER4_AF; /* (1) */
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(GPIO_AFRL_AFRL3 | GPIO_AFRL_AFRL4)); /* (2) */
    // enable clocking
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    // Configure DMA SPI
    /* SPI_RX DMA config */
    /* (1) Peripheral address */
    /* (2) Memory address */
    /* (3) Data size */
    /* (4) Memory increment */
    /*     Peripheral to memory */
    /*     8-bit transfer */
    /*     Overflow IR */
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    DMA1_Channel2->CPAR = (uint32_t)&(SPI1->DR); /* (1) */
    DMA1_Channel2->CMAR = (uint32_t)inbuff; /* (2) */
    DMA1_Channel2->CNDTR = SPIBUFSZ; /* (3) */
    DMA1_Channel2->CCR |= DMA_CCR_MINC | DMA_CCR_EN; /* (4) */
    /* (5) Peripheral address */
    /* (6) Memory address */
    /* (7) Memory increment */
    /*     Memory to peripheral*/
    /*     8-bit transfer */
    /*     Transfer complete IT */
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;
    DMA1_Channel3->CPAR = (uint32_t)&(SPI1->DR); /* (5) */
    DMA1_Channel3->CMAR = (uint32_t)outbuff; /* (6) */
    DMA1_Channel3->CCR |= DMA_CCR_MINC |  DMA_CCR_TCIE | DMA_CCR_DIR; /* (7) */
    NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0);
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    /* Configure SPI */
    /* (1) Master selection, BR: Fpclk/256 CPOL and CPHA at zero (rising first edge) */
    /* (1a) software slave management (SSI inactive) */
    /* (2) TX and RX with DMA, 8-bit Rx fifo */
    /* (3) Enable SPI */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_BR | SPI_CR1_SSM | SPI_CR1_SSI; /* (1) */
    SPI1->CR2 = SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN | SPI_CR2_FRXTH | SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0; /* (2) */
    SPI1->CR1 |= SPI_CR1_SPE; /* (3) */
    SPI_prep_receive();
}

// prepare to receive data
void SPI_prep_receive(){
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    rxrdy = 0;
    (void)SPI1->DR; // read DR and SR to clear OVR flag
    (void)SPI1->SR;
    DMA1_Channel2->CNDTR = SPIBUFSZ;
    DMA1_Channel3->CNDTR = SPIBUFSZ;
    DMA1_Channel2->CCR |= DMA_CCR_EN;
    DMA1_Channel3->CCR |= DMA_CCR_EN;
}

/**
 * @brief SPI_getdata - get data received by DMA & reload receiver
 * @param buf    - buffer for data (with length maxlen) or NULL
 * @param maxlen - (I) - amount of received bytes (or 0 if buffer is empty),
 *                 (O) - amount of real bytes amount in buffer (could be > maxlen if maxlen < SPIBUFSZ)
 * @return 1 if got data
 */
uint8_t SPI_getdata(uint8_t *buf, uint8_t *maxlen){
    if(!rxrdy) return 0;
    //SEND("def\n", 4);
    uint8_t remain = DMA1_Channel2->CNDTR;
    if(remain) return 0;
    rxrdy = 0;
    if(maxlen){
        if(buf && *maxlen) memcpy(buf, inbuff, *maxlen);
        *maxlen = SPIBUFSZ; // bytes received
    }
    return 1;
}

void dma1_channel2_3_isr(){
    if(DMA1->ISR & DMA_ISR_TCIF3){
        DMA1->IFCR |= DMA_IFCR_CTCIF3;
        DMA1_Channel3->CCR &= ~DMA_CCR_EN;
        rxrdy = 1;
    }
    if(DMA1->ISR & DMA_ISR_TEIF2){ // receiver overflow
        DMA1->IFCR |= DMA_IFCR_CTEIF2;
        DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    }
}
