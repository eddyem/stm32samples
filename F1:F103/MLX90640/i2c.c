/*
 * This file is part of the MLX90640 project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "strfunct.h"

extern volatile uint32_t Tms;

// current addresses for read/write (should be set with i2c_set_addr7)
static uint8_t addr7r = 0, addr7w = 0;

/*
 * PB10/PB6 - I2C_SCL, PB11/PB7 - I2C_SDA or remap @ PB8 & PB9
 */
void i2c_setup(){
    I2C1->CR1 = 0;
    I2C1->SR1 = 0;
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    GPIOB->CRL = (GPIOB->CRL & ~(GPIO_CRL_CNF6 | GPIO_CRL_CNF7)) |
            CRL(6, CNF_AFOD | MODE_NORMAL) | CRL(7, CNF_AFOD | MODE_NORMAL);
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    I2C1->CR2 = 8; // FREQR=8MHz, T=125ns
    I2C1->TRISE = 9; // (9-1)*125 = 1mks
    I2C1->CCR = 40; // normal mode, 8MHz/2/40 = 100kHz
    I2C1->CR1 |= I2C_CR1_PE; // enable periph
}

void i2c_set_addr7(uint8_t addr){
	addr7w = addr << 1;
	addr7r = addr7w | 1;
}

// wait for event evt no more than 2 ms
#define I2C_WAIT(evt)  do{ register uint32_t wait4 = Tms + 2;   \
		while(Tms < wait4 && !(evt)) IWDG->KR = IWDG_REFRESH;   \
		if(!(evt)){ret = I2C_TMOUT; goto eotr;}}while(0)
// wait for !busy
#define I2C_LINEWAIT() do{ register uint32_t wait4 = Tms + 2;                   \
    while(Tms < wait4 && (I2C1->SR2 & I2C_SR2_BUSY)) IWDG->KR = IWDG_REFRESH;   \
    if(I2C1->SR2 & I2C_SR2_BUSY){I2C1->CR1 |= I2C_CR1_SWRST; return I2C_LINEBUSY;}\
    }while(0)

// start writing
static i2c_status i2c_7bit_startw(){
    i2c_status ret = I2C_LINEBUSY;
    if(I2C1->CR1 != I2C_CR1_PE) i2c_setup();
    if(I2C1->SR1) I2C1->SR1 = 0; // clear NACK and other problems
    (void) I2C1->SR2;
    I2C_LINEWAIT();
    DBG("linew");
    I2C1->CR1 |= I2C_CR1_START;             // generate start sequence
    I2C_WAIT(I2C1->SR1 & I2C_SR1_SB);       // wait for SB
    DBG("SB");
    (void) I2C1->SR1;                       // clear SB
    I2C1->DR = addr7w;                      // set address
    I2C_WAIT(I2C1->SR1 & I2C_SR1_ADDR);     // wait for ADDR flag (timeout @ NACK)
    DBG("ADDR");
    if(I2C1->SR1 & I2C_SR1_AF){             // NACK
        return I2C_NACK;
    }
    DBG("ACK");
    (void) I2C1->SR2;                       // clear ADDR
    ret = I2C_OK;
eotr:
    return ret;
}

// send data array
i2c_status i2c_7bit_send(const uint8_t *data, int datalen, uint8_t stop){
    i2c_status ret = i2c_7bit_startw();
    if(ret != I2C_OK){
        DBG("NACK!");
        I2C1->CR1 |= I2C_CR1_STOP;
        goto eotr;
    }
    for(int i = 0; i < datalen; ++i){
        I2C1->DR = data[i];
        I2C_WAIT(I2C1->SR1 & I2C_SR1_TXE);
    }
    DBG("GOOD");
    ret = I2C_OK;
    if(datalen || stop) I2C_WAIT(I2C1->SR1 & I2C_SR1_BTF);
eotr:
    if(stop){
        I2C1->CR1 |= I2C_CR1_STOP;          // generate stop event
    }else{DBG("No STOP");}
    return ret;
}

i2c_status i2c_7bit_receive_onebyte(uint8_t *data, uint8_t stop){
    i2c_status ret = I2C_LINEBUSY;
    I2C1->CR1 |= I2C_CR1_START;             // generate start sequence
    I2C_WAIT(I2C1->SR1 & I2C_SR1_SB);       // wait for SB
    DBG("got SB");
    (void) I2C1->SR1;                       // clear SB
    I2C1->DR = addr7r;                      // set address
    DBG("Rx addr");
    I2C_WAIT(I2C1->SR1 & I2C_SR1_ADDR);     // wait for ADDR flag
    DBG("Rx ack");
    I2C1->CR1 &= ~I2C_CR1_ACK;              // clear ACK
    if(I2C1->SR1 & I2C_SR1_AF){             // NACK
        DBG("Rx nak");
        ret = I2C_NACK;
        goto eotr;
    }
    (void) I2C1->SR2;                       // clear ADDR
    DBG("Rx stop");
    if(stop) I2C1->CR1 |= I2C_CR1_STOP;     // program STOP
    I2C_WAIT(I2C1->SR1 & I2C_SR1_RXNE);     // wait for RxNE
    DBG("Rx OK");
    *data = I2C1->DR;                       // read data & clear RxNE
    ret = I2C_OK;
eotr:
    return ret;
}

i2c_status i2c_7bit_receive_twobytes(uint8_t *data){
    i2c_status ret = I2C_LINEBUSY;
    I2C1->CR1 |= I2C_CR1_START | I2C_CR1_POS | I2C_CR1_ACK; // generate start sequence, set pos & ack
    I2C_WAIT(I2C1->SR1 & I2C_SR1_SB);       // wait for SB
    DBG("2 got sb");
    (void) I2C1->SR1;                       // clear SB
    I2C1->DR = addr7r;                      // set address
    I2C_WAIT(I2C1->SR1 & I2C_SR1_ADDR);     // wait for ADDR flag
    DBG("2 ADDR");
    if(I2C1->SR1 & I2C_SR1_AF){             // NACK
        ret = I2C_NACK;
        goto eotr;
    }
    DBG("2 ACK");
    (void) I2C1->SR2;                       // clear ADDR
    I2C1->CR1 &= ~I2C_CR1_ACK;              // clear ACK
    I2C_WAIT(I2C1->SR1 & I2C_SR1_BTF);      // wait for BTF
    DBG("2 BTF");
    I2C1->CR1 |= I2C_CR1_STOP;              // program STOP
    *data++ = I2C1->DR; *data = I2C1->DR;   // read data & clear RxNE
    ret = I2C_OK;
eotr:
    return ret;
}

// receive any amount of bytes
i2c_status i2c_7bit_receive(uint8_t *data, uint16_t nbytes){
    if(nbytes == 0) return I2C_HWPROBLEM;
    I2C_LINEWAIT();
    I2C1->SR1 = 0; // clear previous NACK flag & other error flags
    if(nbytes == 1) return i2c_7bit_receive_onebyte(data, 1);
    else if(nbytes == 2) return i2c_7bit_receive_twobytes(data);
    i2c_status ret = I2C_LINEBUSY;
    I2C1->CR1 |= I2C_CR1_START | I2C_CR1_ACK; // generate start sequence, set pos & ack
    I2C_WAIT(I2C1->SR1 & I2C_SR1_SB);       // wait for SB
    DBG("n got SB");
    (void) I2C1->SR1;                       // clear SB
    I2C1->DR = addr7r;                      // set address
    I2C_WAIT(I2C1->SR1 & I2C_SR1_ADDR);     // wait for ADDR flag
    DBG("n send addr");
    if(I2C1->SR1 & I2C_SR1_AF){             // NACK
        DBG("n NACKed");
        ret = I2C_NACK;
        goto eotr;
    }
    DBG("n ACKed");
    (void) I2C1->SR2;                       // clear ADDR
    for(uint16_t x = nbytes - 3; x > 0; --x){
        I2C_WAIT(I2C1->SR1 & I2C_SR1_RXNE); // wait next byte
        *data++ = I2C1->DR;                 // get data
    }
    DBG("n three left");
    // three bytes remain to be read
    I2C_WAIT(I2C1->SR1 & I2C_SR1_RXNE);     // wait dataN-2
    DBG("n dataN-2");
    I2C_WAIT(I2C1->SR1 & I2C_SR1_BTF);      // wait for BTF
    DBG("n BTF");
    I2C1->CR1 &= ~I2C_CR1_ACK;              // clear ACK
    *data++ = I2C1->DR;                     // read dataN-2
    I2C1->CR1 |= I2C_CR1_STOP;              // program STOP
    *data++ = I2C1->DR;                     // read dataN-1
    I2C_WAIT(I2C1->SR1 & I2C_SR1_RXNE);     // wait next byte
    *data = I2C1->DR;                       // read dataN
    DBG("n got it");
    ret = I2C_OK;
eotr:
    return ret;
}
