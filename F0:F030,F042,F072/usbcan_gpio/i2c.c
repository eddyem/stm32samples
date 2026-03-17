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

#include "gpio.h"
#include "i2c.h"

// fields position in I2C1->TIMINGR
#define I2C_TIMINGR_PRESC_Pos   28
#define I2C_TIMINGR_SCLDEL_Pos  20
#define I2C_TIMINGR_SDADEL_Pos  16
#define I2C_TIMINGR_SCLH_Pos    8
#define I2C_TIMINGR_SCLL_Pos    0

i2c_speed_t curI2Cspeed = I2C_SPEED_10K;
extern volatile uint32_t Tms;
static uint32_t cntr;
volatile uint8_t I2C_scan_mode = 0; // == 1 when I2C is in scan mode
static uint8_t i2caddr  = I2C_ADDREND; // address for `scan`, not active

void i2c_setup(i2c_speed_t speed){
    if(speed >= I2C_SPEED_1M) speed = curI2Cspeed;
    else curI2Cspeed = speed;
    uint8_t PRESC, SCLDEL = 4, SDADEL = 2, SCLH, SCLL;
    I2C1->CR1 = 0;
    // I2C
    RCC->CFGR3 |= RCC_CFGR3_I2C1SW; // use sysclock for timing
    switch(curI2Cspeed){
        case I2C_SPEED_10K:
            PRESC = 0x0B;
            SCLH = 0xC3;
            SCLL = 0xC7;
            break;
        case I2C_SPEED_100K:
            PRESC = 0x0B;
            SCLH = 0x0F;
            SCLL = 0x13;
            break;
        case I2C_SPEED_400K:
            SDADEL = 3;
            SCLDEL = 3;
            PRESC = 5;
            SCLH = 3;
            SCLL = 9;
            break;
        case I2C_SPEED_1M:
        default:
            SDADEL = 0;
            SCLDEL = 1;
            PRESC = 5;
            SCLH = 1;
            SCLL = 3;
            break;
    }
    I2C1->TIMINGR = (PRESC<<I2C_TIMINGR_PRESC_Pos) | (SCLDEL<<I2C_TIMINGR_SCLDEL_Pos) |
                    (SDADEL<<I2C_TIMINGR_SDADEL_Pos) | (SCLH<<I2C_TIMINGR_SCLH_Pos) | (SCLL<< I2C_TIMINGR_SCLL_Pos);
    if(speed < I2C_SPEED_400K){
        SYSCFG->CFGR1 &= ~SYSCFG_CFGR1_I2C_FMP_I2C1;
    }else{ // activate FM+
        SYSCFG->CFGR1 |= SYSCFG_CFGR1_I2C_FMP_I2C1;
    }
    I2C1->ICR = 0xffff; // clear all errors
    I2C1->CR1 = I2C_CR1_PE;
}

void i2c_stop(){
    I2C1->CR1 = 0;
}

/**
 * write command byte to I2C
 * @param addr - device address (TSYS01_ADDR0 or TSYS01_ADDR1)
 * @param data - bytes to write
 * @param nbytes - amount of bytes to write
 * @param stop - to set STOP
 * @return 0 if error
 */
static uint8_t i2c_writes(uint8_t addr, uint8_t *data, uint8_t nbytes, uint8_t stop){
    cntr = Tms;
    I2C1->CR1 = 0; // clear busy flag
    I2C1->ICR = 0x3f38; // clear all errors
    I2C1->CR1 = I2C_CR1_PE;
    while(I2C1->ISR & I2C_ISR_BUSY){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - cntr > I2C_TIMEOUT){
            DBG("Line busy\n");
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
                DBG("NAK\n");
                return 0;
            }
            if(Tms - cntr > I2C_TIMEOUT){
                DBG("Timeout\n");
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

uint8_t i2c_write(uint8_t addr, uint8_t *data, uint8_t nbytes){
    return i2c_writes(addr, data, nbytes, 1);
}

/**
 * read nbytes of data from I2C line
 * `data` should be an array with at least `nbytes` length
 * @return 1 if all OK, 0 if NACK or no device found
 */
static uint8_t i2c_readb(uint8_t addr, uint8_t *data, uint8_t nbytes, uint8_t busychk){
    if(busychk){
        cntr = Tms;
        while(I2C1->ISR & I2C_ISR_BUSY){
            IWDG->KR = IWDG_REFRESH;
            if(Tms - cntr > I2C_TIMEOUT){
                DBG("Line busy\n");
                return 0;  // check busy
        }}
    }
    cntr = Tms;
    while(I2C1->CR2 & I2C_CR2_START){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - cntr > I2C_TIMEOUT){
            DBG("No start\n");
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
                DBG("NAK\n");
                return 0;
            }
            if(Tms - cntr > I2C_TIMEOUT){
                DBG("Timeout\n");
                return 0;
            }
        }
        *data++ = I2C1->RXDR;
    }
    return 1;
 }

uint8_t i2c_read(uint8_t addr, uint8_t *data, uint8_t nbytes){
    return i2c_readb(addr, data, nbytes, 1);
}

// read register reg
uint8_t i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t nbytes){
    if(!i2c_writes(addr, &reg, 1, 0)) return 0;
    return i2c_readb(addr, data, nbytes, 0);
}

void i2c_init_scan_mode(){
    i2caddr = 1;
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
    if(!i2c_read_reg((i2caddr++)<<1, 0, NULL, 0)) return 0;
    return 1;
}
