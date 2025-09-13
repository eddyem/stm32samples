/*
 * This file is part of the i2cscan project.
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
#include "usb.h"

I2C_SPEED curI2Cspeed = LOW_SPEED;
extern volatile uint32_t Tms;
static uint32_t cntr;
static uint8_t i2c_got_DMA_Rx = 0;
volatile uint8_t I2C_scan_mode = 0; // == 1 when I2C is in scan mode
static uint8_t i2caddr  = I2C_ADDREND; // current address in scan mode
static volatile int I2Cbusy = 0, goterr = 0; // busy==1 when DMA active, goterr==1 if 't was error @ last sent
static uint8_t I2Cbuf[256], i2cbuflen = 0; // buffer for DMA tx/rx and its len

// macros for I2C rx/tx
#define DMARXCCR    (DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_TEIE)
#define DMATXCCR    (DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE | DMA_CCR_TEIE)
// macro for I2CCR1
#define I2CCR1      (I2C_CR1_PE | I2C_CR1_RXDMAEN | I2C_CR1_TXDMAEN)

// return 1 if I2Cbusy is set & timeout reached
static inline int isI2Cbusy(){
    cntr = Tms;
    do{
        if(Tms - cntr > I2C_TIMEOUT){ USND("Timeout, DMA transfer in progress?\n"); return 1;}
    }while(I2Cbusy);
    return 0;
}

// GPIO Resources: I2C1_SCL - PB6 (AF4), I2C1_SDA - PB7 (AF4)
void i2c_setup(I2C_SPEED speed){
    if(speed >= CURRENT_SPEED){
        speed = curI2Cspeed;
    }else{
        curI2Cspeed = speed;
    }
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    I2C1->CR1 = 0;
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
    if(speed == HIGH_SPEED){
        // activate "fast mode plus"
        SYSCFG->CFGR1 |= SYSCFG_CFGR1_I2C1_FMP | SYSCFG_CFGR1_I2C_PB6_FMP | SYSCFG_CFGR1_I2C_PB7_FMP;
    }else{
        SYSCFG->CFGR1 &= ~(SYSCFG_CFGR1_I2C1_FMP | SYSCFG_CFGR1_I2C_PB6_FMP | SYSCFG_CFGR1_I2C_PB7_FMP);
    }
    if(speed == LOW_SPEED){ // 100kHz
        I2C1->TIMINGR = (0xf<<28) | (0x04<<20) | (0x03<<16) | (0x12<<8) | (0x15);
        //                 PRESC      SCLDEL        SDADEL        SCLH      SCLL
    }else if(speed == HIGH_SPEED){ // 400kHz
        I2C1->TIMINGR = (0x06<<28) | (0x04<<20) | (0x03<<16) | (0x06<<8) | (0x08);
    }else{ // VERYLOW_SPEED - 10kHz
        I2C1->TIMINGR = (0xf<<28) | (0x04<<20) | (0x03<<16) | (0xc3<<8) | (0xc7);
    }
    I2C1->CR1 = I2CCR1;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    NVIC_EnableIRQ(DMA1_Channel6_IRQn);
    NVIC_EnableIRQ(DMA1_Channel7_IRQn);
    I2Cbusy = 0;
}

// setup DMA for rx (tx==0) or tx (tx==1)
// DMA channels: 7 - I2C1_Rx, 6 - I2C1_Tx
static void i2cDMAsetup(int tx, uint8_t len){
    if(tx){
        DMA1_Channel6->CCR = DMATXCCR;
        DMA1_Channel6->CPAR = (uint32_t) &I2C1->TXDR;
        DMA1_Channel6->CMAR = (uint32_t) I2Cbuf;
        DMA1_Channel6->CNDTR = i2cbuflen = len;
    }else{
        DMA1_Channel7->CCR = DMARXCCR;
        DMA1_Channel7->CPAR = (uint32_t) &I2C1->RXDR;
        DMA1_Channel7->CMAR = (uint32_t) I2Cbuf;
        DMA1_Channel7->CNDTR = i2cbuflen = len;
    }
}

static uint8_t i2c_chk(uint8_t busychk){
    I2C1->CR2 = 0;
    if(busychk){
        cntr = Tms;
        while(I2C1->ISR & I2C_ISR_BUSY){
            IWDG->KR = IWDG_REFRESH;
            if(Tms - cntr > I2C_TIMEOUT){
                USND("i2c_chk: Line busy\n");
                return 0;  // check busy
        }}
    }/*
    cntr = Tms;
    while(I2C1->CR2 & I2C_CR2_START){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - cntr > I2C_TIMEOUT){
            USND("i2c_chk: No start\n");
            return 0; // check start
    }}*/
    return 1;
}

// start writing
static uint8_t i2c_startw(uint8_t addr, uint8_t nbytes, uint8_t stop){
    if(!i2c_chk(1)) return 0;
    I2C1->CR2 = nbytes << 16 | addr;
    if(stop){
        I2C1->CR2 |= I2C_CR2_AUTOEND; // autoend
    }else{
        //I2C1->CR2 &= ~I2C_CR2_AUTOEND;
        //I2C1->CR2 |= I2C_CR2_RELOAD;
    }
    // now start transfer
    I2C1->CR2 |= I2C_CR2_START;
    return 1;
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
    if(!i2c_startw(addr, nbytes, stop)) return 0;
    for(int i = 0; i < nbytes; ++i){
        cntr = Tms;
        while(!(I2C1->ISR & I2C_ISR_TXIS)){ // ready to transmit
            IWDG->KR = IWDG_REFRESH;
            if(I2C1->ISR & I2C_ISR_NACKF){
                I2C1->ICR |= I2C_ICR_NACKCF;
                USND("write_i2cs: NAK\n");
                return 0;
            }
            if(Tms - cntr > I2C_TIMEOUT){
                USND("write_i2cs: Timeout\n");
                return 0;
            }
        }
        I2C1->TXDR = data[i]; // send data
        USND("write_i2cs: "); USND(uhex2str(data[i])); newline();
    }
    cntr = Tms;
    if(stop){
        while(I2C1->ISR & I2C_ISR_BUSY){
            IWDG->KR = IWDG_REFRESH;
            if(Tms - cntr > I2C_TIMEOUT){USND("write_i2cs: Busy timeout\n"); break;}
        }
    }else{ // repeated start
        while(!(I2C1->ISR & I2C_ISR_TC)){
            IWDG->KR = IWDG_REFRESH;
            if(Tms - cntr > I2C_TIMEOUT){
                USND("write_i2cs: TC timeout\n");
                return 0;
            }
        }
    }
    return 1;
}

uint8_t write_i2c(uint8_t addr, uint8_t *data, uint8_t nbytes){
    if(isI2Cbusy()) return 0;
    return write_i2cs(addr, data, nbytes, 1);
}

uint8_t write_i2c_dma(uint8_t addr, uint8_t *data, uint8_t nbytes){
    if(!data || nbytes < 1) return 0;
    if(isI2Cbusy()) return 0;
    memcpy((char*)I2Cbuf, (char*)data, nbytes);
    i2cDMAsetup(1, nbytes);
    goterr = 0;
    if(!i2c_startw(addr, nbytes, 1)) return 0;
    I2Cbusy = 1;
    DMA1_Channel6->CCR = DMATXCCR | DMA_CCR_EN; // start transfer
    return 1;
}

// start reading
static uint8_t i2c_startr(uint8_t addr, uint8_t nbytes, uint8_t busychk){
    if(!i2c_chk(busychk)) return 0;
    // read N bytes
    I2C1->CR2 = (nbytes<<16) | addr /*| I2C_CR2_AUTOEND*/ | I2C_CR2_RD_WRN;
    I2C1->CR2 |= I2C_CR2_START;
    I2C1->CR2 |= I2C_CR2_AUTOEND;
    return 1;
}

/**
 * read nbytes of data from I2C line
 * all functions with `addr` should have addr = address << 1
 * `data` should be an array with at least `nbytes` length
 * @return 1 if all OK, 0 if NACK or no device found
 */
static uint8_t read_i2cb(uint8_t addr, uint8_t *data, uint8_t nbytes, uint8_t busychk){
    if(!i2c_startr(addr, nbytes, busychk)) return 0;
    uint8_t i;
    for(i = 0; i < nbytes; ++i){
        cntr = Tms;
        while(!(I2C1->ISR & I2C_ISR_RXNE)){ // wait for data
            IWDG->KR = IWDG_REFRESH;
            if(I2C1->ISR & I2C_ISR_NACKF){
                I2C1->ICR |= I2C_ICR_NACKCF;
                USND("read_i2_nostart: NAK\n");
                return 0;
            }
            if(Tms - cntr > I2C_TIMEOUT){
                USND("read_i2_nostart: Timeout\n");
                return 0;
            }
        }
        *data++ = I2C1->RXDR;
    }/*
    cntr = Tms;
    while(!(I2C1->ISR & I2C_ISR_TC)){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - cntr > I2C_TIMEOUT){
            USND("read_i2cs: TC timeout\n");
            return 0;
        }
    }
    I2C1->CR2 = I2C_CR2_STOP;*/
    return 1;
}

uint8_t read_i2c(uint8_t addr, uint8_t *data, uint8_t nbytes){
    if(isI2Cbusy()) return 0;
    return read_i2cb(addr, data, nbytes, 1);
}

uint8_t read_i2c_dma(uint8_t addr, uint8_t nbytes){
    if(nbytes < 1) return 0;
    if(isI2Cbusy()) return 0;
    i2cDMAsetup(0, nbytes);
    goterr = 0;
    if(!i2c_startr(addr, nbytes, 1)) return 0;
    I2Cbusy = 1;
    DMA1_Channel7->CCR = DMARXCCR | DMA_CCR_EN; // start transfer
    return 1;
}


// read register reg
uint8_t read_i2c_reg(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t nbytes){
    if(isI2Cbusy()) return 0;
    if(!write_i2cs(addr, &reg, 1, 0)) return 0;
    return read_i2cb(addr, data, nbytes, 0);
}

// read 16bit register reg
uint8_t read_i2c_reg16(uint8_t addr, uint16_t reg16, uint8_t *data, uint8_t nbytes){
    if(isI2Cbusy()) return 0;
    reg16 = __REV16(reg16);
    if(!write_i2cs(addr, (uint8_t*)&reg16, 2, 0)) return 0;
    return read_i2cb(addr, data, nbytes, 0);
}

void i2c_init_scan_mode(){
    i2caddr = 1; // start from 1 as 0 is a broadcast address
    I2C_scan_mode = 1;
}

// return 1 if next addr is active & return in as `addr`
// if addresses are over, return 1 and set addr to I2C_NOADDR
// if scan mode inactive, return 0 and set addr to I2C_NOADDR
int i2c_scan_next_addr(uint8_t *addr){
    if(isI2Cbusy()) return 0;
    *addr = i2caddr;
    if(i2caddr == I2C_ADDREND){
        *addr = I2C_ADDREND;
        I2C_scan_mode = 0;
        return 0;
    }
    /*while(!u3txrdy);
    USND("Addr: "); USND(uhex2str(i2caddr)); USND("\n");
    usart3_sendbuf();*/
    uint8_t byte;
    if(!read_i2c((i2caddr++)<<1, &byte, 1)) return 0;
    return 1;
}

// dump I2Cbuf
void i2c_bufdudump(){
    if(goterr){
        USND("Last transfer ends with error!\n");
        goterr = 0;
    }
    USND("I2C buffer:\n");
    hexdump(USB_sendstr, I2Cbuf, i2cbuflen);
}

void i2c_have_DMA_Rx(){
    if(!i2c_got_DMA_Rx) return;
    i2c_got_DMA_Rx = 0;
    i2c_bufdudump();
}

int i2cdma_haderr(){
    int r = goterr;
    goterr = 0;
    return r;
}

// Rx (7) /Tx (6) interrupts
static void I2C_isr(int rx){
    uint32_t isr = DMA1->ISR;
    DMA_Channel_TypeDef *ch = (rx) ? DMA1_Channel7 : DMA1_Channel6;
    if(isr & (DMA_ISR_TEIF6 | DMA_ISR_TEIF6)) goterr = 1;
    if(rx) i2c_got_DMA_Rx = 1; // last transfer was Rx
    ch->CCR = 0;
    I2Cbusy = 0;
    DMA1->IFCR = 0x0ff00000; // clear all flags for channel6/7
}

void dma1_channel6_isr(){
    I2C_isr(0);
}

void dma1_channel7_isr(){
    I2C_isr(1);
}
