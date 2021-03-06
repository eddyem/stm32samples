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
#include "proto.h"
#include "usb.h"

I2C_SPEED curI2Cspeed = LOW_SPEED;
extern volatile uint32_t Tms;
static uint32_t cntr;
volatile uint8_t I2C_scan_mode = 0; // == 1 when I2C is in scan mode
static uint8_t i2caddr  = I2C_ADDREND; // not active

void i2c_setup(I2C_SPEED speed){
    if(speed >= CURRENT_SPEED){
        speed = curI2Cspeed;
    }else{
        curI2Cspeed = speed;
    }
    I2C1->CR1 = 0;
/*
 * GPIO Resources: I2C1_SCL - PB6, I2C1_SDA - PB7 (AF1)
 */
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(GPIO_AFRL_AFRL6 | GPIO_AFRL_AFRL7)) |
            1 << (6 * 4) | 1 << (7 * 4);
    GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODER6 | GPIO_MODER_MODER7)) |
                    GPIO_MODER_MODER6_AF | GPIO_MODER_MODER7_AF;
    GPIOB->OTYPER |= GPIO_OTYPER_OT_6 | GPIO_OTYPER_OT_7; // both open-drain outputs
    // I2C
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; // timing
    RCC->CFGR3 |= RCC_CFGR3_I2C1SW; // use sysclock for timing
    if(speed == LOW_SPEED){ // 10kHz
        // PRESC=B, SCLDEL=4, SDADEL=2, SCLH=0xC3, SCLL=0xB0
        I2C1->TIMINGR = (0xB<<28) | (4<<20) | (2<<16) | (0xC3<<8) | (0xB0);
    }else if(speed == HIGH_SPEED){ // 100kHz
        I2C1->TIMINGR = (0xB<<28) | (4<<20) | (2<<16) | (0x12<<8) | (0x11);
    }else{ // VERYLOW_SPEED - the lowest speed by STM register: 5.8kHz (presc = 16-1 = 15; )
        I2C1->TIMINGR = (0xf<<28) | (4<<20) | (2<<16) | (0xff<<8) | (0xff);
    }
    I2C1->CR1 = I2C_CR1_PE;
}


/**
 * write command byte to I2C
 * @param addr - device address (TSYS01_ADDR0 or TSYS01_ADDR1)
 * @param data - bytes to write
 * @param nbytes - amount of bytes to write
 * @param stop - to set STOP
 * @return 0 if error
 */
static uint8_t write_i2cs(uint8_t addr, uint8_t *data, uint8_t nbytes, uint8_t stop){
    cntr = Tms;
    I2C1->CR1 = 0; // clear busy flag
    I2C1->ICR = 0x3f38; // clear all errors
    I2C1->CR1 = I2C_CR1_PE;
    while(I2C1->ISR & I2C_ISR_BUSY){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - cntr > I2C_TIMEOUT){
            USND("Line busy\n");
            return 0;  // check busy
    }}
    cntr = Tms;
    while(I2C1->CR2 & I2C_CR2_START){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - cntr > I2C_TIMEOUT){
        return 0; // check start
    }}
    //I2C1->ICR = 0x3f38; // clear all errors
    I2C1->CR2 = nbytes << 16 | addr;
    if(stop) I2C1->CR2 |= I2C_CR2_AUTOEND; // autoend
    // now start transfer
    I2C1->CR2 |= I2C_CR2_START;
    for(int i = 0; i < nbytes; ++i){
        cntr = Tms;
        while(!(I2C1->ISR & I2C_ISR_TXIS)){ // ready to transmit
            IWDG->KR = IWDG_REFRESH;
            if(I2C1->ISR & I2C_ISR_NACKF){
                I2C1->ICR |= I2C_ICR_NACKCF;
                USND("NAK\n");
                return 0;
            }
            if(Tms - cntr > I2C_TIMEOUT){
                USND("Timeout\n");
                return 0;
            }
        }
        I2C1->TXDR = data[i]; // send data
    }
    // wait for data gone
    while(I2C1->ISR & I2C_ISR_BUSY){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - cntr > I2C_TIMEOUT){break;}
    }
    return 1;
}

uint8_t write_i2c(uint8_t addr, uint8_t *data, uint8_t nbytes){
    return write_i2cs(addr, data, nbytes, 1);
}

/**
 * read nbytes of data from I2C line
 * `data` should be an array with at least `nbytes` length
 * @return 1 if all OK, 0 if NACK or no device found
 */
static uint8_t read_i2cb(uint8_t addr, uint8_t *data, uint8_t nbytes, uint8_t busychk){
    if(busychk){
        cntr = Tms;
        while(I2C1->ISR & I2C_ISR_BUSY){
            IWDG->KR = IWDG_REFRESH;
            if(Tms - cntr > I2C_TIMEOUT){
                USND("Line busy\n");
                return 0;  // check busy
        }}
    }
    cntr = Tms;
    while(I2C1->CR2 & I2C_CR2_START){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - cntr > I2C_TIMEOUT){
            USND("No start\n");
            return 0; // check start
    }}
    // read N bytes
    I2C1->CR2 = (nbytes<<16) | addr | 1 | I2C_CR2_AUTOEND | I2C_CR2_RD_WRN;
    I2C1->CR2 |= I2C_CR2_START;
    uint8_t i;
    for(i = 0; i < nbytes; ++i){
        cntr = Tms;
        while(!(I2C1->ISR & I2C_ISR_RXNE)){ // wait for data
            IWDG->KR = IWDG_REFRESH;
            if(I2C1->ISR & I2C_ISR_NACKF){
                I2C1->ICR |= I2C_ICR_NACKCF;
                USND("NAK\n");
                return 0;
            }
            if(Tms - cntr > I2C_TIMEOUT){
                USND("Timeout\n");
                return 0;
            }
        }
        *data++ = I2C1->RXDR;
    }
    return 1;
 }

uint8_t read_i2c(uint8_t addr, uint8_t *data, uint8_t nbytes){
    return read_i2cb(addr, data, nbytes, 1);
}

// read register reg
uint8_t read_i2c_reg(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t nbytes){
    if(!write_i2cs(addr, &reg, 1, 0)) return 0;
    return read_i2cb(addr, data, nbytes, 0);
}

void i2c_init_scan_mode(){
    i2caddr = 0;
    I2C_scan_mode = 1;
}

// return 1 if next addr is active & return in as `addr`
// if addresses are over, return 1 and set addr to I2C_NOADDR
// if scan mode inactive, return 0 and set addr to I2C_NOADDR
int i2c_scan_next_addr(uint8_t *addr){
    *addr = i2caddr;
    if(i2caddr == I2C_ADDREND){
        *addr = I2C_ADDREND;
        I2C_scan_mode = 0;
        return 0;
    }
    if(!read_i2c_reg((i2caddr++)<<1, 0, NULL, 0)) return 0;
    return 1;
}
