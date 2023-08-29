/*
 *                                                                                                  geany_encoding=koi8-r
 * i2c.c
 *
 * Copyright 2017 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "hardware.h"
#include "i2c.h"

/**
 * I2C for TSYS01
 * Speed <= 400kHz (200)
 * t_SCLH > 21ns
 * t_SCLL > 21ns
 * while reading, sends NACK
 * after reading get 24bits of T value, we need upper 2 bytes: ADC16 = ADC>>8
 * T =    (-2) * k4 * 10^{-21} * ADC16^4
 *      +   4  * k3 * 10^{-16} * ADC16^3
 *      + (-2) * k2 * 10^{-11} * ADC16^2
 *      +   1  * k1 * 10^{-6}  * ADC16
 *      +(-1.5)* k0 * 10^{-2}
 * All coefficiens are in registers:
 * k4 - 0xA2, k3 - 0xA4, k2 - 0xA6, k1 - 0xA8, k0 - 0xAA
 */

extern volatile uint32_t Tms;
static uint32_t cntr;

/**
 * write command byte to I2C
 * @param addr - device address (TSYS01_ADDR0 or TSYS01_ADDR1)
 * @param data - byte to write
 * @return 0 if error
 */
uint8_t write_i2c(uint8_t addr, uint8_t data){
    cntr = Tms;
    I2C1->ICR = 0x3f38; // clear all errors
    while(I2C1->ISR & I2C_ISR_BUSY){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - cntr > I2C_TIMEOUT){
        return 0;  // check busy
    }}
    cntr = Tms;
    while(I2C1->CR2 & I2C_CR2_START){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - cntr > I2C_TIMEOUT){
        return 0; // check start
    }}
    //I2C1->ICR = 0x3f38; // clear all errors
    I2C1->CR2 = 1<<16 | addr | I2C_CR2_AUTOEND;  // 1 byte, autoend
    // now start transfer
    I2C1->CR2 |= I2C_CR2_START;
    cntr = Tms;
    while(!(I2C1->ISR & I2C_ISR_TXIS)){ // ready to transmit
        IWDG->KR = IWDG_REFRESH;
        if(I2C1->ISR & I2C_ISR_NACKF){
            I2C1->ICR |= I2C_ICR_NACKCF;
            return 0;
        }
        if(Tms - cntr > I2C_TIMEOUT){
            return 0;
        }
    }
    I2C1->TXDR = data; // send data
    // wait for data gone
    while(I2C1->ISR & I2C_ISR_BUSY){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - cntr > I2C_TIMEOUT){break;}
    }
    return 1;
}

/**
 * read nbytes (2 or 3) of data from I2C line
 * @return 1 if all OK, 0 if NACK or no device found
 */
uint8_t read_i2c(uint8_t addr, uint32_t *data, uint8_t nbytes){
    uint32_t result = 0;
    cntr = Tms;
    //MSG("read_i2c\n");
    while(I2C1->ISR & I2C_ISR_BUSY){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - cntr > I2C_TIMEOUT){
        return 0;  // check busy
    }}
    cntr = Tms;
    while(I2C1->CR2 & I2C_CR2_START){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - cntr > I2C_TIMEOUT){
        return 0; // check start
    }}
  //  I2C1->ICR = 0x3f38; // clear all errors
    // read N bytes
    I2C1->CR2 = (nbytes<<16) | addr | 1 | I2C_CR2_AUTOEND | I2C_CR2_RD_WRN;
    I2C1->CR2 |= I2C_CR2_START;
    uint8_t i;
    cntr = Tms;
    for(i = 0; i < nbytes; ++i){
        while(!(I2C1->ISR & I2C_ISR_RXNE)){ // wait for data
            IWDG->KR = IWDG_REFRESH;
            if(I2C1->ISR & I2C_ISR_NACKF){
                I2C1->ICR |= I2C_ICR_NACKCF;
                return 0;
            }
            if(Tms - cntr > I2C_TIMEOUT){
                return 0;
            }
        }
        result = (result << 8) | I2C1->RXDR;
    }
    *data = result;
    return 1;
 }
