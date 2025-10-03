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
#include <string.h> // memcpy

#include "usb_dev.h"
#ifdef EBUG
#include "strfunc.h"
#endif

spiStatus spi_status = SPI_NOTREADY;
#define WAITX(x)  do{volatile uint32_t  wctr = 0; while((x) && (++wctr < 360000)) IWDG->KR = IWDG_REFRESH; if(wctr==360000){ DBG("timeout"); return 0;}}while(0)

// init SPI @ ~280kHz (36MHz/128)
void spi_setup(){
    SPI1->CR1 = 0; // clear EN
    SPI1->CR2 = 0;
    // PB3 - SCK, BP4 - MISO, PB5 - MOSI; AF5 @PB3-5
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(GPIO_AFRL_AFRL3 | GPIO_AFRL_AFRL4 | GPIO_AFRL_AFRL5)) |
                    AFRf(5, 3) | AFRf(5, 4) | AFRf(5, 5);
    GPIOB->MODER = (GPIOB->MODER & (MODER_CLR(3) & MODER_CLR(4) & MODER_CLR(5))) |
                    MODER_AF(3) | MODER_AF(4) | MODER_AF(5);
    RCC->APB1RSTR = RCC_APB2RSTR_SPI1RST;
    RCC->APB1RSTR = 0; // clear reset
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    // software slave management (without hardware NSS pin); RX only; Baudrate = 0b110 - fpclk/128
    // CPOL=1, CPHA=1:
    SPI1->CR1 = SPI_CR1_CPOL | SPI_CR1_CPHA | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_MSTR | SPI_CR1_BR_2 | SPI_CR1_BR_1;
    // hardware NSS management, RXNE after 8bit; 8bit transfer (default)
    // DS=8bit; RXNE generates after 8bit of data in FIFO;
    SPI1->CR2 = SPI_CR2_SSOE | SPI_CR2_FRXTH | SPI_CR2_DS_2|SPI_CR2_DS_1|SPI_CR2_DS_0;
    spi_status = SPI_READY;
    DBG("SPI works");
}

void spi_onoff(uint8_t on){
    if(on) SPI1->CR1 |= SPI_CR1_SPE;
    else SPI1->CR1 &= ~SPI_CR1_SPE;
}

// turn off given SPI channel and release GPIO
void spi_deinit(){
    SPI1->CR1 = 0;
    SPI1->CR2 = 0;
    RCC->APB2ENR &= ~RCC_APB2ENR_SPI1EN;
    GPIOB->AFR[0] = GPIOB->AFR[0] & ~(GPIO_AFRL_AFRL3 | GPIO_AFRL_AFRL4 | GPIO_AFRL_AFRL5);
    GPIOB->MODER = GPIOB->MODER & (MODER_CLR(3) & MODER_CLR(4) & MODER_CLR(5));
    spi_status = SPI_NOTREADY;
}

uint8_t spi_waitbsy(){
    WAITX(SPI1->SR & SPI_SR_BSY);
    return 1;
}

/**
 * @brief spi_writeread - send data over SPI (change data array with received bytes)
 * @param data - data to write/read
 * @param n - length of data
 * @return 0 if failed
 */
uint8_t spi_writeread(uint8_t *data, uint8_t n){
    if(spi_status != SPI_READY || !data || !n){
        DBG("not ready");
        return 0;
    }
    // clear SPI Rx FIFO
    spi_onoff(TRUE);
    for(int i = 0; i < 4; ++i) (void) SPI1->DR;
    for(int x = 0; x < n; ++x){
        WAITX(!(SPI1->SR & SPI_SR_TXE));
        *((volatile uint8_t*)&SPI1->DR) = data[x];
        WAITX(!(SPI1->SR & SPI_SR_RXNE));
        data[x] = *((volatile uint8_t*)&SPI1->DR);
    }
    spi_onoff(FALSE); // turn off SPI
    return 1;
}

// read data through SPI
uint8_t spi_read(uint8_t *data, uint8_t n){
    if(spi_status != SPI_READY || !data || !n){
        DBG("not ready");
        return 0;
    }
    // clear SPI Rx FIFO
    for(int i = 0; i < 4; ++i) (void) SPI1->DR;
    spi_onoff(TRUE);
    for(int x = 0; x < n; ++x){
        WAITX(!(SPI1->SR & SPI_SR_RXNE));
        data[x] = *((volatile uint8_t*)&SPI1->DR);
    }
    spi_onoff(FALSE); // turn off SPI
    return 1;
}

