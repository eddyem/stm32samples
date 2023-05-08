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

// init SPI2 to work with and without DMA
// ILI9341: SCL 0->1; CS=0; command - DC=0, data - DC=1; 1 dummy clock pulse before 24/32 bit data read
// Channel 4 - SPI2 Rx
// Channel 5 - SPI2 Tx
void spi_setup(){
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    // Baudrate = 0b011 - fpclk/16 = 2MHz; software slave management (without hardware NSS pin)
    SPI2->CR1 = /*SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE |*/ SPI_CR1_MSTR | SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_SSM | SPI_CR1_SSI;
    // 8bit; RXNE generates after 8bit of data in FIFO
    SPI2->CR2 = SPI_CR2_FRXTH | SPI_CR2_DS_2|SPI_CR2_DS_1|SPI_CR2_DS_0 /*| SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN*/;
    spi_status = SPI_READY;
    SPI2->CR1 |= SPI_CR1_SPE;
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
        //WAITX(!(SPI2->SR & SPI_SR_RXNE));
        //data[x] = SPI2->DR; // clear RXNE after last things
    }
    return 1;
}

/**
 * @brief spi_send_dma - send data over SPI2 through DMA
 * @param data - data to read
 * @param n - length of data
 * @return 0 if failed
 */
int spi_write_dma(const uint8_t _U_ *data, uint32_t _U_ n){
    if(spi_status != SPI_READY) return 0;
    return 0;
}

/**
 * @brief spi_read - read SPI2 data
 * @param data - data to read
 * @param n - length of data
 * @return n
 */
int spi_read(uint8_t _U_ *data, uint32_t _U_ n){
    if(spi_status != SPI_READY){
        DBG("not ready");
        return 0;
    }
    //SPI2->CR1 &= ~SPI_CR1_BIDIOE; // Rx
    while(SPI2->SR & SPI_SR_RXNE) (void) SPI2->DR;
    for(uint32_t x = 0; x < n; ++x){
        WAITX(!(SPI2->SR & SPI_SR_TXE));
        SPIDR = 0;
        WAITX(!(SPI2->SR & SPI_SR_RXNE));
        data[x] = SPI2->DR;
    }
    //SPI2->CR1 |= SPI_CR1_BIDIOE; // turn off clocking
    return 1;
}

/**
 * @brief spi_read_dma - read SPI2 data through DMA
 * @param data - data to read
 * @param n - length of data
 * @return n
 */
int spi_read_dma(uint8_t _U_ *data, uint32_t _U_ n){
    if(spi_status != SPI_READY) return 0;
    return 0;
}

