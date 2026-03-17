/*
 * This file is part of the usbcangpio project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include  <stm32f0.h>

#include "hardware.h"
#include "spi.h"

// get best prescaler to fit given frequency
static uint16_t get_baudrate_prescaler(uint32_t speed_hz){
    uint32_t freq = peripherial_clock;
    uint32_t best_i = 7;
    uint32_t best_err = 0xFFFFFFFF;
    for(int i = 0; i < 8; i++){
        freq >>= 1;
        uint32_t err = (freq > speed_hz) ? (freq - speed_hz) : (speed_hz - freq);
        if(err < best_err){
            best_err = err;
            best_i = i;
        }else if(err > best_err) break;
    }
    return best_i;
}


// Master, 8bit, CPOL=0, CPHA=0, MSB first
void spi_setup(uint32_t speed){ // speed in Hz
    RCC->APB2RSTR |= RCC_APB2RSTR_SPI1RST;
    RCC->APB2RSTR = 0;
    SPI1->CR1 = 0;
    uint16_t br = get_baudrate_prescaler(speed);
    SPI1->CR1 = SPI_CR1_MSTR | (br << 3) | SPI_CR1_SSM | SPI_CR1_SSI;
    SPI1->CR2 = SPI_CR2_SSOE;
    SPI1->CR1 |= SPI_CR1_SPE;
}

void spi_stop(){
    SPI1->CR1 &= ~SPI_CR1_SPE;
}

// return -1 if SPI isn't run or got error
int spi_transfer(const uint8_t *tx, uint8_t *rx, int len){
    if(len < 1 || !(SPI1->CR1 & SPI_CR1_SPE)) return -1;
    int i;
    for(i = 0; i < len; ++i){
        uint32_t timeout = 1000000;
        while(!(SPI1->SR & SPI_SR_TXE)){
            if (--timeout == 0) return -1; // error by timeout: TX isn't ready
        }
        uint8_t out = (tx) ? tx[i] : 0;
        *(uint8_t*)&SPI1->DR = out; // запись в DR
        timeout = 1000000;
        while(!(SPI1->SR & SPI_SR_RXNE)){
            if(--timeout == 0) return 0;
        }
        uint8_t in = *(uint8_t*)&SPI1->DR; // чтение из DR
        if(rx) rx[i] = in;
    }
    //while(SPI1->SR & SPI_SR_BSY){ }
    return i;
}
