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
#include "stm32f0.h"
#include "hardware.h"
#include "i2c.h"
#include "usart.h"

static uint32_t cntr;

static uint8_t wri2c(uint8_t byte){
    I2C1->CR2 = byte;// | I2C_CR2_AUTOEND;
    if(byte & 1) I2C1->CR2 |= I2C_CR2_RD_WRN;
    // now start transfer
    I2C1->CR2 |= I2C_CR2_START;
    if(I2C1->ISR & I2C_ISR_RXNE) (void) I2C1->RXDR;
    cntr = Tms;
    while((I2C1->ISR & I2C_ISR_TC) == 0) if(Tms - cntr > I2C_TIMEOUT) return 0;
    if(I2C1->ISR & I2C_ISR_RXNE) (void) I2C1->RXDR;
    I2C1->ICR = 0xff; // clear all error flags
    return 1;
}

/**
 * write command byte to I2C
 * @param addr - device address (TSYS01_ADDR0 or TSYS01_ADDR1)
 * @param data - byte to write
 * @return 0 if error
 */
uint8_t write_i2c(const uint8_t *commands, uint8_t nbytes){
    uint8_t r = 1;
    cntr = Tms;
    while(I2C1->ISR & I2C_ISR_BUSY) if(Tms - cntr > I2C_TIMEOUT) return 0;  // check busy
    cntr = Tms;
    while(I2C1->CR2 & I2C_CR2_START) if(Tms - cntr > I2C_TIMEOUT) return 0; // check start
    for(uint8_t i = 0; i < nbytes; ++i){
        if(!wri2c(commands[i])){
            r = 0;
            break;
        }
    }
    I2C1->CR2 |= I2C_CR2_STOP;
    return r;
}

/**
 * read nbytes (0..4) of data from I2C line
 * @return 1 if all OK, 0 if NACK or no device found
 */
uint8_t read_i2c(uint8_t command, uint8_t *data){
    cntr = Tms;
    while(I2C1->ISR & I2C_ISR_BUSY) if(Tms - cntr > 5) return 0;  // check busy
    cntr = Tms;
    while(I2C1->CR2 & I2C_CR2_START) if(Tms - cntr > 5) return 0; // check start
    // read 1 byte
    if(command & 1){
        I2C1->CR2 = (1<<16) | command | I2C_CR2_RD_WRN;
    }else{
        wri2c(command);
        /*
        I2C1->CR2 = command;
        I2C1->CR2 |= I2C_CR2_START;
        cntr = Tms;
        while((I2C1->ISR & I2C_ISR_TC) == 0) if(Tms - cntr > I2C_TIMEOUT) return 0;
        */
        I2C1->CR2 = (1<<16) |0xff | I2C_CR2_RD_WRN;
    }
    I2C1->CR2 |= I2C_CR2_START;
    I2C1->CR2 |= I2C_CR2_STOP;
    cntr = Tms;
    while(Tms == cntr);
/*
    while(!(I2C1->ISR & I2C_ISR_RXNE)){ // wait for data
        if(I2C1->ISR & I2C_ISR_NACKF){
            I2C1->ICR |= I2C_ICR_NACKCF;
            return 0;
        }
        if(Tms - cntr > 5) return 0;
    }
*/
    if(I2C1->ISR & I2C_ISR_RXNE){
        *data = I2C1->RXDR;
        return 1;
    }
    return 0;
 }
