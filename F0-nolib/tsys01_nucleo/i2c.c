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

/*
 * Resources: I2C1_SCL - PA9, I2C1_SDA - PA10
 * GPIOA->AFR[1] AF4 -- GPIOA->AFR[1] &= ~0xff0, GPIOA->AFR[1] |= 0x440
 */
extern volatile uint32_t Tms;
static uint32_t cntr;

void i2c_setup(){
    I2C1->CR1 = 0;
    // GPIO
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // clock
    GPIOA->AFR[1] &= ~0xff0; // alternate function F4 for PA9/PA10
    GPIOA->AFR[1] |= 0x440;
    GPIOA->OTYPER |= GPIO_OTYPER_OT_9 | GPIO_OTYPER_OT_10; // opendrain
    GPIOA->MODER &= ~(GPIO_MODER_MODER9 | GPIO_MODER_MODER10);
    GPIOA->MODER |= GPIO_MODER_MODER9_AF | GPIO_MODER_MODER10_AF; // alternate function
    // I2C
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; // timing
    RCC->CFGR3 |= RCC_CFGR3_I2C1SW; // use sysclock for timing
    // Clock = 6MHz, 0.16(6)us, need 5us (*30)
    // PRESC=4 (f/5), SCLDEL=0 (t_SU=5/6us), SDADEL=0 (t_HD=5/6us), SCLL,SCLH=14 (2.(3)us)
    I2C1->TIMINGR = (4<<28) | (14<<8) | (14); // 0x40000e0e
    I2C1->CR1 = I2C_CR1_PE;// | I2C_CR1_RXIE; // Enable I2C & (interrupt on receive - not supported yet)
}

/**
 * write command byte to I2C
 * @param addr - device address (TSYS01_ADDR0 or TSYS01_ADDR1)
 * @param data - byte to write
 * @return 0 if error
 */
uint8_t write_i2c(uint8_t addr, uint8_t data){
    cntr = Tms;
    while(I2C1->ISR & I2C_ISR_BUSY) if(Tms - cntr > 5) return 0;  // check busy
    cntr = Tms;
    while(I2C1->CR2 & I2C_CR2_START) if(Tms - cntr > 5) return 0; // check start
    I2C1->CR2 = 1<<16 | addr | I2C_CR2_AUTOEND;  // 1 byte, autoend
    // now start transfer
    I2C1->CR2 |= I2C_CR2_START;
    cntr = Tms;
    while(!(I2C1->ISR & I2C_ISR_TXIS)){ // ready to transmit
        if(I2C1->ISR & I2C_ISR_NACKF){
            I2C1->ICR |= I2C_ICR_NACKCF;
            return 0;
        }
        if(Tms - cntr > 5) return 0;
    }
    I2C1->TXDR = data; // send data
    return 1;
}

/**
 * read nbytes (2 or 3) of data from I2C line
 * @return 1 if all OK, 0 if NACK or no device found
 */
uint8_t read_i2c(uint8_t addr, uint32_t *data, uint8_t nbytes){
    uint32_t result = 0;
    cntr = Tms;
    while(I2C1->ISR & I2C_ISR_BUSY) if(Tms - cntr > 5) return 0;  // check busy
    cntr = Tms;
    while(I2C1->CR2 & I2C_CR2_START) if(Tms - cntr > 5) return 0; // check start
    // read N bytes
    I2C1->CR2 = (nbytes<<16) | addr | 1 | I2C_CR2_AUTOEND | I2C_CR2_RD_WRN;
    I2C1->CR2 |= I2C_CR2_START;
    uint8_t i;
    cntr = Tms;
    for(i = 0; i < nbytes; ++i){
        while(!(I2C1->ISR & I2C_ISR_RXNE)){ // wait for data
            if(I2C1->ISR & I2C_ISR_NACKF){
                I2C1->ICR |= I2C_ICR_NACKCF;
                return 0;
            }
            if(Tms - cntr > 5) return 0;
        }
        result = (result << 8) | I2C1->RXDR;
    }
    *data = result;
    return 1;
 }


