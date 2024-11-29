/*
 * This file is part of the I2Cscan project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "i2c.h"

#ifdef EBUG
#undef DBG
#define DBG(x)
#endif

extern volatile uint32_t Tms;

// current addresses for read/write (should be set with i2c_set_addr7)
static uint8_t addr7r = 0, addr7w = 0;

void i2c_set_addr7(uint8_t addr){
	addr7w = addr << 1;
	addr7r = addr7w | 1;
}

static uint8_t aflag = 0;
static uint32_t sctr = 0;

/*
 * PB10/PB6 - I2C_SCL, PB11/PB7 - I2C_SDA or remap @ PB8 & PB9
 */
void i2c_setup(){
    ++sctr;
    //RCC->APB1RSTR = RCC_APB1RSTR_I2C1RST; // reset I2C
    //RCC->APB1RSTR = 0;
    I2C1->CR1 = 0;
    I2C1->SR1 = 0;
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    GPIOB->CRL = (GPIOB->CRL & ~(GPIO_CRL_CNF6 | GPIO_CRL_CNF7)) |
                 CRL(6, CNF_AFOD | MODE_NORMAL) | CRL(7, CNF_AFOD | MODE_NORMAL);
    I2C1->CR2 = 8; // FREQR=8MHz, T=125ns
    I2C1->TRISE = 9; // (9-1)*125 = 1mks
    I2C1->CCR = 40; // normal mode, 8MHz/2/40 = 100kHz
    I2C1->CR1 = I2C_CR1_PE; // enable periph
}

#if 0
// wait for event evt no more than 2 ms
#define I2C_WAIT(evt)  do{ register uint32_t wait4 = Tms + 2;   \
		while(Tms < wait4 && !(evt)) IWDG->KR = IWDG_REFRESH;   \
		if(!(evt)){ret = I2C_TMOUT; goto eotr;}}while(0)
// wait for !busy
#define I2C_LINEWAIT() do{ register uint32_t wait4 = Tms + 2;                   \
    while(Tms < wait4 && (I2C1->SR2 & I2C_SR2_BUSY)) IWDG->KR = IWDG_REFRESH;   \
    if(I2C1->SR2 & I2C_SR2_BUSY){I2C1->CR1 |= I2C_CR1_SWRST; return I2C_LINEBUSY;}\
    }while(0)
#endif

// wait for event evt
#define I2C_WAIT(evt)  do{ register uint32_t xx = 2000000;   \
while(--xx && !(evt)) IWDG->KR = IWDG_REFRESH;   \
    if(!(evt)){ret = I2C_TMOUT; goto eotr;}}while(0)
// wait for !busy
#define I2C_LINEWAIT() do{ register uint32_t xx = 2000000;                   \
    while(--xx && (I2C1->SR2 & I2C_SR2_BUSY)) IWDG->KR = IWDG_REFRESH;   \
    if(I2C1->SR2 & I2C_SR2_BUSY){I2C1->CR1 |= I2C_CR1_SWRST; return I2C_LINEBUSY;}\
}while(0)



static uint8_t bytes_remaining = 0;

i2c_status i2c_start(){
    i2c_status ret = I2C_LINEBUSY;
    aflag = 44;
    I2C_LINEWAIT();
    I2C1->CR1 |= I2C_CR1_START; // generate start sequence, set pos & ack
    aflag = 1;
    I2C_WAIT(I2C1->SR1 & I2C_SR1_SB);       // wait for SB
    (void) I2C1->SR1;                       // clear SB
    ret = I2C_OK;
    aflag = 2;
eotr:
    return ret;
}

i2c_status i2c_sendaddr(uint8_t addr, uint8_t nread){
    i2c_set_addr7(addr);
    i2c_status ret = I2C_LINEBUSY;
    I2C1->DR = (nread) ? addr7r : addr7w;   // set address
    aflag = 3;
    I2C_WAIT(I2C1->SR1 & I2C_SR1_ADDR);     // wait for ADDR flag
    aflag = 4;
    if(I2C1->SR1 & I2C_SR1_AF){             // NACK
        ret = I2C_NACK;
        goto eotr;
    }
    aflag = 5;
    ret = I2C_OK;
    bytes_remaining = nread;
    if(nread == 1) I2C1->CR1 &= ~I2C_CR1_ACK; // clear ACK
    else if(nread >= 2) I2C1->CR1 |= I2C_CR1_ACK;
    (void) I2C1->SR2;                       // clear ADDR
    if(nread == 1) I2C1->CR1 |= I2C_CR1_STOP;
eotr:
    return ret;
}

i2c_status i2c_sendbyte(uint8_t data){
    i2c_status ret = I2C_LINEBUSY;
    I2C1->DR = data;                        // init data send register
    //I2C_WAIT(I2C1->SR1 & I2C_SR1_TXE);      // wait for TxE (timeout when NACK)
    aflag = 6;
    I2C_WAIT(I2C1->SR1 & I2C_SR1_BTF);
    aflag = 7;
    ret = I2C_OK;
eotr:
    return ret;
}

i2c_status i2c_readbyte(uint8_t *data){
    i2c_status ret = I2C_LINEBUSY;
    aflag = 8;
    I2C_WAIT(I2C1->SR1 & I2C_SR1_RXNE);     // wait for RxNE
    aflag = 9;
    if(--bytes_remaining == 1){
        I2C1->CR1 &= ~I2C_CR1_ACK;
        I2C1->CR1 |= I2C_CR1_STOP;
    }
    *data = I2C1->DR;                       // read data & clear RxNE
    ret = I2C_OK;
eotr:
    return ret;
}

i2c_status i2c_stop(){
    i2c_status ret = I2C_LINEBUSY;
    aflag = 10;
    //I2C_WAIT(I2C1->SR1 & (I2C_SR1_TXE | I2C_SR1_BTF));
    //aflag = 11;
    I2C1->CR1 |= I2C_CR1_STOP;
    ret = I2C_OK;
//eotr:
    return ret;
}
