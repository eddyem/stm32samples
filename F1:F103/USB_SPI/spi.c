/*
 * This file is part of the LED_screen project.
 * Copyright 2019 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

// memcpy hangs -> use my own

#include "spi.h"
#include "hardware.h"
#include "usb.h"
#include "proto.h"

static void mymemcpy(uint8_t *dest, uint8_t *src, int len){
    while(len--) *dest++ = *src++;
}

// CR1 register default values, can be changed in 'proto.c'
uint32_t SPI_CR1 = SPI_CR1_MSTR | SPI_CR1_BR | SPI_CR1_SSM | SPI_CR1_SSI;

spiStatus SPI_status = SPI_NOTREADY;
static uint8_t inbuff[SPIBUFSZ] = {0}, outbuff[SPIBUFSZ], lastlen = 0;

void spi_setup(){
    // configure SPI (transmit only)
    RCC->APB2ENR |= SPI_APB2; // Enable the peripheral clock SPI1
    RCC->AHBENR |= DMA_SPI_AHBENR; // and DMA1
    // master, no slave select, BR=F/16, CPOL/CPHA - polarity.
    SPIx->CR1 = SPI_CR1;
    // Rx/Tx with DMA
    SPIx->CR2 = SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN;
    // setup SPI1 DMA
    // Tx
    DMA_SPI_TxChannel->CPAR = (uint32_t)&(SPIx->DR); // hardware
    // memory increment, mem->hw, TC irq
    DMA_SPI_TxChannel->CCR |= DMA_CCR_MINC | DMA_CCR_DIR;
    // Rx
    DMA_SPI_RxChannel->CPAR = (uint32_t)&(SPIx->DR); // hardware
    DMA_SPI_RxChannel->CCR |= DMA_CCR_MINC | DMA_CCR_TCIE; // mem inc, hw->mem, TC irq
    NVIC_SetPriority(DMA_SPI_Rx_IRQ, 0);
    NVIC_EnableIRQ(DMA_SPI_Rx_IRQ);
    SPI_status = SPI_READY;
    SPIx->CR1 |= SPI_CR1_SPE; // enable SPI
}

/**
 * @brief SPI_transmit - transmit data over SPI DMA
 * @param buf - data to transmit
 * @param len - its length
 * @return amount of transmitted data
 */
uint8_t SPI_transmit(const uint8_t *buf, uint8_t len){
    if(!buf || !len) return 0; // bad data format
    if(SPI_status != SPI_READY) return 0; // spi not ready to transmit data
#if 0
    for(uint8_t x = 0; x < len; ++x){
        while(!(SPI1->SR & SPI_SR_TXE));
        SPI1->DR = buf[x];
        USB_send(u2hexstr(buf[x])); USB_send(" -> ");
        while(!(SPI1->SR & SPI_SR_BSY));
        while(!(SPI1->SR & SPI_SR_RXNE));
        inbuff[x] = SPI1->DR;
        USB_send(u2hexstr(inbuff[x])); USB_send("\n");
    }
    lastlen = len;
    return len;
#endif
    if(len > SPIBUFSZ) len = SPIBUFSZ; // buflen too much
    mymemcpy(outbuff, (uint8_t*)buf, len);
    DMA_SPI_TxChannel->CCR &=~ DMA_CCR_EN;
    DMA_SPI_RxChannel->CCR &=~ DMA_CCR_EN;
    // refresh broken CMAR
    DMA_SPI_TxChannel->CMAR = (uint32_t)outbuff;
    DMA_SPI_RxChannel->CMAR = (uint32_t)inbuff;
    // set CNDTR
    DMA_SPI_TxChannel->CNDTR = len;
    DMA_SPI_RxChannel->CNDTR = len;
    SPI_status = SPI_BUSY;
    lastlen = len;
    DMA_SPI_RxChannel->CCR |= DMA_CCR_EN;
    DMA_SPI_TxChannel->CCR |= DMA_CCR_EN;
    return len;

}

/**
 * @brief SPI_receive - get received data
 * @param buf - buffer with len >= maxlen
 * @param maxlen (io) - `buf` length
 * @return amount of received bytes
 */
uint8_t SPI_receive(uint8_t *buf, uint8_t maxlen){
    if(SPI_status != SPI_READY) return 0;
    if(lastlen == 0) return 0;
    /*if(DMA_SPI_RxChannel->CNDTR){
        USB_send("Not all received\n");
        return 0; // not all received yet
    }*/
    if(lastlen < maxlen) maxlen = lastlen;
    mymemcpy(buf, inbuff, maxlen);
    lastlen = 0;
    return maxlen;
}

// SPI1 DMA Rx interrupt
void DMA_SPI_Rx_ISR(){
    if(DMA_SPI->ISR & DMA_SPI_TCIF){
        DMA_SPI->IFCR = DMA_SPI_CTCIF; /* Clear TC flag */
        SPI_status = SPI_READY;
    }
}
