/*
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

#include <stm32f3.h>
#include <string.h>

#include "i2c.h"
#include "strfunc.h" // hexdump
#include "usb_dev.h"

i2c_speed_t i2c_curspeed = I2C_SPEED_AMOUNT;
extern volatile uint32_t Tms;
static uint32_t cntr;
volatile uint8_t i2c_scanmode = 0; // == 1 when I2C is in scan mode
static uint8_t i2caddr  = I2C_ADDREND; // current address in scan mode
static uint8_t I2Cbuf[I2C_BUFSIZE];

// GPIO Resources: I2C1_SCL - PB6 (AF4), I2C1_SDA - PB7 (AF4)
void i2c_setup(i2c_speed_t speed){
    uint8_t PRESC, SCLDEL = 0x04, SDADEL = 0x03, SCLH, SCLL; // I2C1->TIMINGR fields
    switch(speed){
        case I2C_SPEED_10K:
            PRESC = 0x0F;
            SCLH = 0xDA;
            SCLL = 0xE0;
        break;
        case I2C_SPEED_100K:
            PRESC = 0x0F;
            SCLH = 0x13;
            SCLL = 0x16;
        break;
        case I2C_SPEED_400K:
            PRESC = 0x07;
            SCLH = 0x08;
            SCLL = 0x09;
        break;
        case I2C_SPEED_1M:
            SDADEL = 1;
            SCLDEL = 2;
            PRESC = 0x3;
            SCLH = 0x4;
            SCLL = 0x6;
        break;
        /*case I2C_SPEED_2M:
            SDADEL = 0;
            SCLDEL = 1;
            PRESC = 0x0;
            SCLH = 0x1;
            SCLL = 0x2;
        break;*/
        default:
            USND("Wrong I2C speed!");
            return; // wrong speed
    }
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    I2C1->CR1 = 0; // disable I2C for setup
    I2C1->ICR = 0x3f38; // clear all errors
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(GPIO_AFRL_AFRL6 | GPIO_AFRL_AFRL7)) |
                AFRf(4, 6) | AFRf(4, 7);
    GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODER6 | GPIO_MODER_MODER7)) |
                    GPIO_MODER_MODER6_AF | GPIO_MODER_MODER7_AF;
    GPIOB->PUPDR = (GPIOB->PUPDR & !(GPIO_PUPDR_PUPDR6 | GPIO_PUPDR_PUPDR7)) |
            GPIO_PUPDR6_PU | GPIO_PUPDR7_PU; // pullup (what if there's no external pullup?)
    GPIOB->OTYPER |= GPIO_OTYPER_OT_6 | GPIO_OTYPER_OT_7; // both open-drain outputs
    // I2C (default timing from sys clock - 72MHz)
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; // clocking
    if(speed < I2C_SPEED_400K){ // slow cpeed - common mode
        SYSCFG->CFGR1 &= ~(SYSCFG_CFGR1_I2C1_FMP | SYSCFG_CFGR1_I2C_PB6_FMP | SYSCFG_CFGR1_I2C_PB7_FMP);
    }else{ // activate "fast mode plus"
        SYSCFG->CFGR1 |= SYSCFG_CFGR1_I2C1_FMP | SYSCFG_CFGR1_I2C_PB6_FMP | SYSCFG_CFGR1_I2C_PB7_FMP;
    }
    I2C1->TIMINGR = (PRESC<<I2C_TIMINGR_PRESC_Pos) | (SCLDEL<<I2C_TIMINGR_SCLDEL_Pos) |
                    (SDADEL<<I2C_TIMINGR_SDADEL_Pos) | (SCLH<<I2C_TIMINGR_SCLH_Pos) | (SCLL<< I2C_TIMINGR_SCLL_Pos);
    I2C1->CR1 = I2C_CR1_PE;
    i2c_curspeed = speed;
}

// wait until bit set or clear; return 1 if OK, 0 in case of timeout
static uint8_t waitISRbit(uint32_t bit, uint8_t isset){
    uint32_t waitwhile = (isset) ? 0 : bit; // wait until !=
    //const char *errmsg = NULL;
    cntr = Tms;
    //if(bit != I2C_ISR_RXNE){ U("ISR wait "); U(uhex2str(bit)); USND(isset ? "set" : "reset"); }
    while((I2C1->ISR & bit) == waitwhile){
        IWDG->KR = IWDG_REFRESH;
        if(I2C1->ISR & I2C_ISR_NACKF){
            //errmsg = "NAK";
            goto goterr;
        }
        if(Tms - cntr > I2C_TIMEOUT){
            //errmsg = "timeout";
            goto goterr;
        }
    }
    return 1;
goterr:
    /*U("wait ISR bit: "); USND(errmsg);
    U("I2c->ISR = "); USND(uhex2str(I2C1->ISR));*/
    I2C1->ICR = 0xff;
    return 0;
}

// start writing
static uint8_t i2c_startw(uint8_t addr, uint8_t nbytes, uint8_t stop){
    if(!waitISRbit(I2C_ISR_BUSY, 0)) return 0;
    uint32_t cr2 = nbytes << 16 | addr | I2C_CR2_START;
    if(stop) cr2 |= I2C_CR2_AUTOEND;
    // now start transfer
    I2C1->CR2 = cr2;
    return 1;
}

/**
 * write command byte to I2C
 * @param addr - device address
 * @param data - bytes to write
 * @param nbytes - amount of bytes to write
 * @param stop - to set STOP
 * @return 0 if error
 */
static uint8_t i2c_writes(uint8_t addr, uint8_t *data, uint8_t nbytes, uint8_t stop){
    if(!i2c_startw(addr, nbytes, stop)) return 0;
    for(int i = 0; i < nbytes; ++i){
        cntr = Tms;
        while(!(I2C1->ISR & I2C_ISR_TXIS)){ // ready to transmit
            IWDG->KR = IWDG_REFRESH;
            if(I2C1->ISR & I2C_ISR_NACKF){
                I2C1->ICR |= I2C_ICR_NACKCF;
                //USND("i2c_writes: NAK");
                return 0;
            }
            if(Tms - cntr > I2C_TIMEOUT){
                //USND("i2c_writes: Timeout");
                return 0;
            }
        }
        I2C1->TXDR = data[i]; // send data
        //U("i2c_writes: "); USND(uhex2str(data[i]));
    }
    cntr = Tms;
    if(stop){
        if(!waitISRbit(I2C_ISR_BUSY, 0)) return 0;
    }else{ // repeated start
        if(!waitISRbit(I2C_ISR_TC, 1)) return 0;
    }
    return 1;
}

uint8_t i2c_write(uint8_t addr, uint8_t *data, uint8_t nbytes){
    return i2c_writes(addr, data, nbytes, 1);
}

// start reading of `nbytes` from `addr`; if `start`==`, set START
static uint8_t i2c_startr(uint8_t addr, uint8_t nbytes, uint8_t start){
    uint32_t cr2 = addr | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND | (nbytes << 16);
    I2C1->CR2 = (start) ? cr2 | I2C_CR2_START : cr2;
    return 1;
}

/**
 * read nbytes of data from I2C line
 * all functions with `addr` should have addr = address << 1
 * `data` should be an array with at least `nbytes` length
 * @return 1 if all OK, 0 if NACK or no device found
 */
static uint8_t *i2c_readb(uint8_t addr, uint8_t nbytes){
    uint8_t *bptr = I2Cbuf;
    if(!i2c_startr(addr, nbytes, 1)) return NULL;
    for(int i = 0; i < nbytes; ++i){
        if(!waitISRbit(I2C_ISR_RXNE, 1)) goto tmout;
        *bptr++ = I2C1->RXDR;
    }
    return I2Cbuf;
tmout:
    //USND("read I2C: Timeout");
    return NULL;
}

uint8_t *i2c_read(uint8_t addr, uint8_t nbytes){
    if(!waitISRbit(I2C_ISR_BUSY, 0)) return 0;
    return i2c_readb(addr, nbytes);
}

// read register reg
uint8_t *i2c_read_regs(uint8_t addr, uint8_t reg, uint8_t nbytes){
    if(!waitISRbit(I2C_ISR_BUSY, 0)) return NULL;
    if(!i2c_writes(addr, &reg, 1, 0)) return NULL;
    return i2c_readb(addr, nbytes);
}

void i2c_init_scan_mode(){
    i2caddr = 1; // start from 1 as 0 is a broadcast address
    i2c_scanmode = 1;
}

// return 1 if next addr is active & return in as `addr`
// if addresses are over, return 1 and set addr to I2C_NOADDR
// if scan mode inactive, return 0 and set addr to I2C_NOADDR
int i2c_scan_next_addr(uint8_t *addr){
    *addr = i2caddr;
    if(i2caddr == I2C_ADDREND){
        *addr = I2C_ADDREND;
        i2c_scanmode = 0;
        return 0;
    }
    if(!i2c_read((i2caddr++)<<1, 1)) return 0;
    return 1;
}
