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
#include "usb_dev.h"

i2c_speed_t i2c_curspeed = I2C_SPEED_AMOUNT;
extern volatile uint32_t Tms;
static uint32_t cntr;
volatile uint8_t i2c_scanmode = 0; // == 1 when I2C is in scan mode
static volatile uint8_t i2c_got_DMA = 0; // got DMA data
static uint8_t i2caddr  = I2C_ADDREND; // current address in scan mode
static volatile int I2Cbusy = 0, goterr = 0; // busy==1 when DMA active, goterr==1 if 't was error @ last sent
static uint16_t I2Cbuf[I2C_BUFSIZE];
static uint16_t i2cbuflen = 0; // buffer for DMA rx and its len
static volatile uint16_t dma_remain = 0; // remain bytes of DMA read/write

// macros for I2C rx/tx
#define DMARXCCR    (DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_TEIE)
#define DMATXCCR    (DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE | DMA_CCR_TEIE)
// macro for I2CCR1
#define I2CCR1      (I2C_CR1_PE | I2C_CR1_RXDMAEN | I2C_CR1_TXDMAEN)

// return 1 if I2Cbusy is set & timeout reached
static inline int isI2Cbusy(){
    cntr = Tms;
    do{
        if(Tms - cntr > I2C_TIMEOUT){ U("Timeout, DMA transfer in progress?"); return 1;}
    }while(I2Cbusy);
    return 0;
}

static void swapbytes(uint16_t *data, uint16_t datalen){
    if(!datalen) return;
    for(int i = 0; i < datalen; ++i)
        data[i] = __REV16(data[i]);
}

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
        case I2C_SPEED_2M:
            SDADEL = 0;
            SCLDEL = 1;
            PRESC = 0x0;
            SCLH = 0x1;
            SCLL = 0x2;
        break;
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
                    MODER_AF(6) | MODER_AF(7);
    GPIOB->PUPDR = (GPIOB->PUPDR & !(GPIO_PUPDR_PUPDR6 | GPIO_PUPDR_PUPDR7)) |
            PUPD_PU(6) | PUPD_PU(7); // pullup (what if there's no external pullup?)
    GPIOB->OTYPER |= OTYPER_OD(6) | OTYPER_OD(7); // both open-drain outputs
    GPIOB->OSPEEDR = (GPIOB->OSPEEDR & OSPEED_CLR(6) & OSPEED_CLR(7)) |
            OSPEED_HI(6) | OSPEED_HI(7);
    // I2C (default timing from sys clock - 72MHz)
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; // clocking
    if(speed < I2C_SPEED_400K){ // slow cpeed - common mode
        SYSCFG->CFGR1 &= ~(SYSCFG_CFGR1_I2C1_FMP | SYSCFG_CFGR1_I2C_PB6_FMP | SYSCFG_CFGR1_I2C_PB7_FMP);
    }else{ // activate "fast mode plus"
        SYSCFG->CFGR1 |= SYSCFG_CFGR1_I2C1_FMP | SYSCFG_CFGR1_I2C_PB6_FMP | SYSCFG_CFGR1_I2C_PB7_FMP;
    }
    I2C1->TIMINGR = (PRESC<<I2C_TIMINGR_PRESC_Pos) | (SCLDEL<<I2C_TIMINGR_SCLDEL_Pos) |
                    (SDADEL<<I2C_TIMINGR_SDADEL_Pos) | (SCLH<<I2C_TIMINGR_SCLH_Pos) | (SCLL<< I2C_TIMINGR_SCLL_Pos);
    I2C1->CR1 = I2CCR1;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    NVIC_EnableIRQ(DMA1_Channel6_IRQn);
    NVIC_EnableIRQ(DMA1_Channel7_IRQn);
    I2Cbusy = 0;
    i2c_curspeed = speed;
}

// setup DMA for rx (tx==0) or tx (tx==1)
// DMA channels: 7 - I2C1_Rx, 6 - I2C1_Tx
static void i2cDMAsetup(int tx, uint16_t len){
    i2cbuflen = len;
    if(len > 255) len = 255;
    if(tx){
        DMA1_Channel6->CCR = DMATXCCR;
        DMA1_Channel6->CPAR = (uint32_t) &I2C1->TXDR;
        DMA1_Channel6->CMAR = (uint32_t) I2Cbuf;
        DMA1_Channel6->CNDTR = len;
    }else{
        DMA1_Channel7->CCR = DMARXCCR;
        DMA1_Channel7->CPAR = (uint32_t) &I2C1->RXDR;
        DMA1_Channel7->CMAR = (uint32_t) I2Cbuf;
        DMA1_Channel7->CNDTR = len;
    }
}

// wait until bit set or clear; return 1 if OK, 0 in case of timeout
static uint8_t waitISRbit(uint32_t bit, uint8_t isset){
    uint32_t waitwhile = (isset) ? 0 : bit; // wait until !=
    cntr = Tms;
    while((I2C1->ISR & bit) == waitwhile){
        IWDG->KR = IWDG_REFRESH;
        if(I2C1->ISR & I2C_ISR_NACKF){
            goto goterr;
        }
        if(Tms - cntr > I2C_TIMEOUT){
            goto goterr;
        }
    }
    return 1;
goterr:
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
                return 0;
            }
            if(Tms - cntr > I2C_TIMEOUT){
                return 0;
            }
        }
        I2C1->TXDR = data[i]; // send data
    }
    cntr = Tms;
    if(stop){
        if(!waitISRbit(I2C_ISR_BUSY, 0)) return 0;
    }else{ // repeated start
        if(!waitISRbit(I2C_ISR_TC, 1)) return 0;
    }
    return 1;
}

uint8_t i2c_write(uint8_t addr, uint16_t *data, uint8_t nwords){
    if(nwords < 1 || nwords > 127) return 0;
    if(isI2Cbusy()) return 0;
    uint16_t nbytes = nwords << 1;
    swapbytes(data, nwords);
    return i2c_writes(addr, (uint8_t*)data, nbytes, 1);
}

uint8_t i2c_write_dma16(uint8_t addr, uint16_t *data, uint8_t nwords){
    if(!data || nwords < 1 || nwords > 127) return 0;
    if(isI2Cbusy()) return 0;
    uint16_t nbytes = nwords << 1;
    swapbytes(data, nwords);
    i2cDMAsetup(1, nbytes);
    goterr = 0;
    if(!i2c_startw(addr, nbytes, 1)) return 0;
    I2Cbusy = 1;
    DMA1_Channel6->CCR = DMATXCCR | DMA_CCR_EN; // start transfer
    return 1;
}

// start reading of `nbytes` from `addr`; if `start`==`, set START
static uint8_t i2c_startr(uint8_t addr, uint16_t nbytes, uint8_t start){
    uint32_t cr2 = addr | I2C_CR2_RD_WRN;
    if(nbytes > 255) cr2 |= I2C_CR2_RELOAD | (0xff0000);
    else cr2 |= I2C_CR2_AUTOEND | (nbytes << 16);
    I2C1->CR2 = (start) ? cr2 | I2C_CR2_START : cr2;
    return 1;
}

/**
 * read nbytes of data from I2C line
 * all functions with `addr` should have addr = address << 1
 * `data` should be an array with at least `nbytes` length
 * @return 1 if all OK, 0 if NACK or no device found
 */
static uint8_t *i2c_readb(uint8_t addr, uint16_t nbytes){
    uint8_t start = 1;
    uint8_t *bptr = (uint8_t*)I2Cbuf;
    while(nbytes){
        if(!i2c_startr(addr, nbytes, start)) return NULL;
        if(nbytes < 256){
            for(int i = 0; i < nbytes; ++i){
                if(!waitISRbit(I2C_ISR_RXNE, 1)) return NULL;
                *bptr++ = I2C1->RXDR;
            }
            break;
        }else while(!(I2C1->ISR & I2C_ISR_TCR)){ // until first part read
            if(!waitISRbit(I2C_ISR_RXNE, 1)) return NULL;
            *bptr++ = I2C1->RXDR;
        }
        nbytes -= 255;
        start = 0;
    }
    return (uint8_t*)I2Cbuf;
}

uint8_t *i2c_read(uint8_t addr, uint16_t nbytes){
    if(isI2Cbusy() || !waitISRbit(I2C_ISR_BUSY, 0) || nbytes < 1 || nbytes > I2C_BUFSIZE*2) return 0;
    return i2c_readb(addr, nbytes);
}

static uint8_t dmard(uint8_t addr, uint16_t nbytes){
    if(nbytes < 1 || nbytes > I2C_BUFSIZE*2) return 0;
    i2cDMAsetup(0, nbytes);
    goterr = 0;
    i2c_got_DMA = 0;
    (void) I2C1->RXDR; // avoid wrong first byte
    DMA1_Channel7->CCR = DMARXCCR | DMA_CCR_EN; // init DMA before START sequence
    if(!i2c_startr(addr, nbytes, 1)) return 0;
    dma_remain = nbytes > 255 ? nbytes - 255 : 0; // remainder after first read finish
    I2Cbusy = 1;
    return 1;
}

uint8_t i2c_read_dma16(uint8_t addr, uint16_t nwords){
    if(nwords > I2C_BUFSIZE) return 0; // what if `nwords` is very large? we should check it
    if(isI2Cbusy() || !waitISRbit(I2C_ISR_BUSY, 0)) return 0;
    return dmard(addr, nwords<<1);
}

// read 16bit register reg
uint16_t *i2c_read_reg16(uint8_t addr, uint16_t reg16, uint16_t nwords, uint8_t isdma){
    if(isI2Cbusy() || !waitISRbit(I2C_ISR_BUSY, 0) || nwords < 1 || nwords > I2C_BUFSIZE) return 0;
    reg16 = __REV16(reg16);
    if(!i2c_writes(addr, (uint8_t*)&reg16, 2, 0)) return NULL;
    if(isdma){
        if(dmard(addr, nwords<<1)) return I2Cbuf;
        return NULL;
    }
    if(!i2c_readb(addr, nwords<<1)) return NULL;
    swapbytes((uint16_t*)I2Cbuf, nwords);
    return (uint16_t*)I2Cbuf;
}

void i2c_init_scan_mode(){
    i2caddr = 1; // start from 1 as 0 is a broadcast address
    i2c_scanmode = 1;
}

// return 1 if next addr is active & return in as `addr`
// if addresses are over, return 1 and set addr to I2C_NOADDR
// if scan mode inactive, return 0 and set addr to I2C_NOADDR
int i2c_scan_next_addr(uint8_t *addr){
    if(isI2Cbusy()) return 0;
    *addr = i2caddr;
    if(i2caddr == I2C_ADDREND){
        *addr = I2C_ADDREND;
        i2c_scanmode = 0;
        return 0;
    }
    if(!i2c_read((i2caddr++)<<1, 1)) return 0;
    return 1;
}

// dump I2Cbuf
void i2c_bufdudump(){
    if(goterr){
        USND("DMARDERR");
        goterr = 0;
    }
    if(i2cbuflen < 1) return;
    USND("DMARD=");
    hexdump16(USB_sendstr, (uint16_t*)I2Cbuf, i2cbuflen);
}

// get DMA buffer with conversion to little-endian (if transfer was for 16-bit)
uint16_t *i2c_dma_getbuf(uint16_t *len){
    if(!i2c_got_DMA || i2cbuflen < 1) return NULL;
    i2c_got_DMA = 0;
    i2cbuflen >>= 1; // for hexdump16 - now buffer have uint16_t!
    swapbytes((uint16_t*)I2Cbuf, i2cbuflen);
    if(len) *len = i2cbuflen;
    return I2Cbuf;
}

int i2c_dma_haderr(){
    int r = goterr;
    goterr = 0;
    return r;
}

int i2c_busy(){ return I2Cbusy;}

// Rx (7) /Tx (6) interrupts
static void I2C_isr(int rx){
    uint32_t isr = DMA1->ISR;
    DMA_Channel_TypeDef *ch = (rx) ? DMA1_Channel7 : DMA1_Channel6;
    ch->CCR &= ~DMA_CCR_EN; // clear enable for further settings
    if(isr & (DMA_ISR_TEIF6 | DMA_ISR_TEIF7)){
        goterr = 1; goto ret;
    }
    if(dma_remain){ // receive/send next portion
        uint16_t len = (dma_remain > 255) ? 255 : dma_remain;
        ch->CNDTR = len;
        if(rx){
            if(!i2c_startr(0, dma_remain, 0)){
                goterr = 1; goto ret;
            }
            ch->CMAR += 255;
        }
        dma_remain -= len;
        ch->CCR |= DMA_CCR_EN;
        DMA1->IFCR = DMA_IFCR_CTCIF6 | DMA_IFCR_CTCIF7;
        return;
    }else if(rx) i2c_got_DMA = 1; // last transfer was Rx and all data read
ret:
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
