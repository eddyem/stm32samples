/*
 * This file is part of the MAX7219 project.
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

#include "spi.h"
#include "hardware.h"

spiStatus SPI_status = SPI_NOTREADY;

// 16 bit SPI transfer setup
void SPI_setup(){
    // setup SPI GPIO - alternate function PP (PA4 - NSS, PA5 - SCK, PA7 - MOSI)
    GPIOA->CRL |= CRL(5, CNF_AFPP|MODE_FAST) | CRL(7, CNF_AFPP|MODE_FAST);
    // configure SPI (transmit only)
    /* Enable the peripheral clock SPI1 */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    /* Configure SPI1 in master */
    /* (1) Master selection, BR: Fpclk/16
           CPOL and CPHA at zero (rising first edge), 16 bit (DFF) */
    /* (2) TX with DMA, slave select output disabled (software managed - SPI_CR1_SSM | SPI_CR1_SSI?) */
    /* (3) Enable SPI1 */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_DFF; /* (1) */
    SPI1->CR2 = SPI_CR2_TXDMAEN; /* (2) */
    // setup SPI1 DMA
    /* Enable the peripheral clock DMA1 */
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    /* DMA1 Channel3 SPI1_TX config */
    /* (5) Peripheral address */
    /* (7) Memory increment */
    /*     Memory to peripheral */
    /*     16-bit transfer (both memory & periph) */
    /*     Transfer complete IRQ enable */
    DMA_SPI_Channel->CPAR = (uint32_t)&(SPI1->DR); /* (5) */
    DMA_SPI_Channel->CCR |= DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_TCIE; /* (7) */
    NVIC_SetPriority(DMA1_Channel3_IRQn, 0);
    NVIC_EnableIRQ(DMA1_Channel3_IRQn);
    //NVIC_EnableIRQ(SPI1_IRQn);
    SPI_status = SPI_READY;
    SPI1->CR1 |= SPI_CR1_SPE; /* (3) */
}

/**
 * @brief SPI_transmit - transmit data over SPI DMA
 * @param buf - data to transmit
 * @param len - its length
 * @return 0 if all OK
 */
uint8_t SPI_transmit(const uint16_t *buf, uint8_t len){
    if(!buf || !len) return 1; // bad data format
    if(SPI_status != SPI_READY) return 2; // spi not ready to transmit data
    DMA_SPI_Channel->CCR &=~ DMA_CCR_EN;
    DMA_SPI_Channel->CMAR = (uint32_t)buf;
    DMA_SPI_Channel->CNDTR = len;
    SPI_status = SPI_BUSY;
    DMA_SPI_Channel->CCR |= DMA_CCR_EN;
    return 0;
}

// blocking transmission of one data portion
void SPI_blocktransmit(const uint16_t data){
    while(SPI_status == SPI_BUSY);
    *(uint16_t *)&(SPI1->DR) = data;
    while(!(SPI1->SR & SPI_SR_TXE));
    while(SPI1->SR & SPI_SR_BSY);
}

// SPI DMA Tx interrupt
void dma1_channel3_isr(){
  if(DMA1->ISR & DMA_ISR_TCIF3){
    DMA1->IFCR |= DMA_IFCR_CTCIF3; // Clear TC flag
    SPI_status = SPI_READY;
  }
}
