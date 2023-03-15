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
#ifdef EBUG
#include "usb.h"
#endif
#include "proto.h"

/*
static void mymemcpy(uint8_t *dest, uint8_t *src, int len){
    while(len--) *dest++ = *src++;
}*/

// CR1 register default values, can be changed in 'proto.c'
uint32_t SPI_CR1 = SPI_CR1_MSTR | SPI_CR1_BR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_CPHA | SPI_CR1_CPOL;

spiStatus SPI_status = SPI_NOTREADY;

void spi_setup(){
    RCC->APB2ENR |= SPI_APB2; // Enable the peripheral clock SPI1
    // master, no slave select, BR=F/16, CPOL/CPHA - polarity.
    SPIx->CR1 = SPI_CR1;
    SPI_status = SPI_READY;
    SPIx->CR1 |= SPI_CR1_SPE; // enable SPI
}

volatile uint32_t wctr;
#define WAITX(x)  do{wctr = 0; while((x) && (++wctr < 360000)) IWDG->KR = IWDG_REFRESH; if(wctr==360000) return -1;}while(0)

/**
 * @brief SPI_transmit - transmit data over SPI DMA
 * @param buf - data to transmit
 * @param len - its length
 * @return amount of transmitted data or -1 if error
 */
uint8_t SPI_transmit(uint8_t *buf, uint8_t len){
    if(!buf || !len) return 0; // bad data format
    if(SPI_status != SPI_READY) return 0; // spi not ready to transmit data
#ifdef EBUG
    USB_send("SPI send "); USB_send(u2str(len));
    USB_send("bytes, data: ");
#endif
    for(uint8_t x = 0; x < len; ++x){
        WAITX(!(SPI1->SR & SPI_SR_TXE));
        SPI1->DR = buf[x];
        WAITX(!(SPI1->SR & SPI_SR_BSY));
#ifdef EBUG
        USB_send(u2hexstr(buf[x])); USB_send(", ");
#endif
        WAITX(!(SPI1->SR & SPI_SR_RXNE));
        buf[x] = SPI1->DR;
        for(int ctr = 0; ctr < 3600; ++ctr) IWDG->KR = IWDG_REFRESH; // ~100mks delay
    }
#ifdef EBUG
    USB_send("\nReceive: ");
    for(int i = 0; i < len; ++i){
        USB_send(u2hexstr(buf[i])); USB_send(", ");
    }
    USB_send("\n");
#endif
    return len;
}

