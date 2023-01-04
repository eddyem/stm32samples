/*
 * This file is part of the F1_testbrd project.
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
#include "proto.h"
#include "usart.h"
#include "usb.h"

I2C_SPEED curI2Cspeed = LOW_SPEED;
extern volatile uint32_t Tms;
volatile uint8_t I2C_scan_mode = 0; // == 1 when I2C is in scan mode
static uint8_t i2caddr  = I2C_ADDREND; // current address for scan mode (not active)
static uint8_t addr7r = 0, addr7w = 0;

void i2c_set_addr7(uint8_t addr){
#ifdef EBUG
     usart_send("Change I2C address to ");
     usart_send(uhex2str(addr));
     usart_putchar('\n');
#endif
	addr7w = addr << 1;
	addr7r = addr7w | 1;
}

/*
 * GPIO Resources: I2C1_SCL - PB6, I2C1_SDA - PB7
 */
void i2c_setup(I2C_SPEED speed){
    if(speed >= CURRENT_SPEED){
        speed = curI2Cspeed;
    }else{
        curI2Cspeed = speed;
    }
    I2C1->CR1 = 0;
    I2C1->SR1 = 0;
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    GPIOB->CRL = (GPIOB->CRL & ~(GPIO_CRL_CNF6 | GPIO_CRL_CNF7)) |
            CRL(6, CNF_AFOD | MODE_NORMAL) | CRL(7, CNF_AFOD | MODE_NORMAL);
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    I2C1->CR2 = 8; // FREQR=8MHz, T=125ns
    I2C1->TRISE = 9; // (9-1)*125 = 1mks
    if(speed == LOW_SPEED){ // 10kHz
        I2C1->CCR = 400;
    }else if(speed == HIGH_SPEED){ // 100kHz
        I2C1->CCR = 40; // normal mode, 8MHz/2/40 = 100kHz
    }else{ // VERYLOW_SPEED - 976.8Hz
        I2C1->CCR = 0xfff;
    }
    I2C1->CR1 = I2C_CR1_PE;
}

// wait for event evt no more than 2 ms
#define I2C_WAIT(evt)  do{ register uint32_t wait4 = Tms + 2;   \
		while(Tms < wait4 && !(evt)) IWDG->KR = IWDG_REFRESH;   \
		if(!(evt)){DBG("WAIT!\n"); return FALSE;}}while(0)
// wait for !busy
#define I2C_LINEWAIT() do{ register uint32_t wait4 = Tms + 2;                   \
    while(Tms < wait4 && (I2C1->SR2 & I2C_SR2_BUSY)) IWDG->KR = IWDG_REFRESH;   \
    if(I2C1->SR2 & I2C_SR2_BUSY){I2C1->CR1 |= I2C_CR1_SWRST; DBG("LINE!\n"); return FALSE;}\
    }while(0)


// start writing, return FALSE @ error
static int i2c_7bit_startw(){
    if(I2C1->CR1 != I2C_CR1_PE) i2c_setup(CURRENT_SPEED);
    if(I2C1->SR1) I2C1->SR1 = 0; // clear NACK and other problems
    (void) I2C1->SR2;
    I2C_LINEWAIT();
    DBG("linew\n");
    I2C1->CR1 |= I2C_CR1_START;             // generate start sequence
    I2C_WAIT(I2C1->SR1 & I2C_SR1_SB);       // wait for SB
    DBG("SB\n");
    (void) I2C1->SR1;                       // clear SB
    I2C1->DR = addr7w;                      // set address
    I2C_WAIT(I2C1->SR1 & I2C_SR1_ADDR);     // wait for ADDR flag (timeout @ NACK)
    DBG("ADDR\n");
    if(I2C1->SR1 & I2C_SR1_AF){             // NACK
        return FALSE;
    }
    DBG("ACK\n");
    (void) I2C1->SR2;                       // clear ADDR
    return TRUE;
}

/**
 * send one byte in 7bit address mode
 * @param data - data to write
 * @param stop - ==1 to send stop event
 * @return TRUE if OK
 *
static int i2c_7bit_send_onebyte(uint8_t data, uint8_t stop){
    int ret = i2c_7bit_startw();
    if(!ret){
        I2C1->CR1 |= I2C_CR1_STOP;
        return FALSE;
    }
    I2C1->DR = data;                        // init data send register
    DBG("TxE\n");
    I2C_WAIT(I2C1->SR1 & I2C_SR1_TXE);      // wait for TxE (timeout when NACK)
    DBG("OK\n");
    if(stop){
        I2C_WAIT(I2C1->SR1 & I2C_SR1_BTF);  // wait for BTF
        DBG("BTF\n");
    }
    if(stop){
        I2C1->CR1 |= I2C_CR1_STOP;          // generate stop event
    }else{ DBG("No STOP\n");}
    return TRUE;
}*/

int i2c_7bit_receive_onebyte(uint8_t *data, uint8_t stop){
    I2C1->CR1 |= I2C_CR1_START;             // generate start sequence
    I2C_WAIT(I2C1->SR1 & I2C_SR1_SB);       // wait for SB
    DBG("got SB\n");
    (void) I2C1->SR1;                       // clear SB
    I2C1->DR = addr7r;                      // set address
    DBG("Rx addr\n");
    I2C_WAIT(I2C1->SR1 & I2C_SR1_ADDR);     // wait for ADDR flag
    DBG("Rx ack\n");
    I2C1->CR1 &= ~I2C_CR1_ACK;              // clear ACK
    if(I2C1->SR1 & I2C_SR1_AF){             // NACK
        DBG("Rx nak\n");
        return FALSE;
    }
    (void) I2C1->SR2;                       // clear ADDR
    DBG("Rx stop\n");
    if(stop) I2C1->CR1 |= I2C_CR1_STOP;     // program STOP
    I2C_WAIT(I2C1->SR1 & I2C_SR1_RXNE);     // wait for RxNE
    DBG("Rx OK\n");
    *data = I2C1->DR;                       // read data & clear RxNE
    return TRUE;
}

int i2c_7bit_receive_twobytes(uint8_t *data){
    I2C1->CR1 |= I2C_CR1_START | I2C_CR1_POS | I2C_CR1_ACK; // generate start sequence, set pos & ack
    I2C_WAIT(I2C1->SR1 & I2C_SR1_SB);       // wait for SB
    DBG("2 got sb\n");
    (void) I2C1->SR1;                       // clear SB
    I2C1->DR = addr7r;                      // set address
    I2C_WAIT(I2C1->SR1 & I2C_SR1_ADDR);     // wait for ADDR flag
    DBG("2 ADDR\n");
    if(I2C1->SR1 & I2C_SR1_AF){             // NACK
        return FALSE;
    }
    DBG("2 ACK\n");
    (void) I2C1->SR2;                       // clear ADDR
    I2C1->CR1 &= ~I2C_CR1_ACK;              // clear ACK
    I2C_WAIT(I2C1->SR1 & I2C_SR1_BTF);      // wait for BTF
    DBG("2 BTF\n");
    I2C1->CR1 |= I2C_CR1_STOP;              // program STOP
    *data++ = I2C1->DR; *data = I2C1->DR;   // read data & clear RxNE
    return TRUE;
}

/**
 * write command byte to I2C
 * @param data - bytes to write
 * @param nbytes - amount of bytes to write
 * @param stop - to set STOP
 * @return 0 if error
 */
static uint8_t write_i2cs(uint8_t *data, uint8_t nbytes, uint8_t stop){
    int ret = i2c_7bit_startw();
    if(!ret){
        DBG("NACK!\n");
        I2C1->CR1 |= I2C_CR1_STOP;
        return FALSE;
    }
    for(int i = 0; i < nbytes; ++i){
        I2C1->DR = data[i];
        I2C_WAIT(I2C1->SR1 & I2C_SR1_TXE);
    }
    DBG("GOOD\n");
    if(nbytes) I2C_WAIT(I2C1->SR1 & I2C_SR1_BTF);
    if(stop) I2C1->CR1 |= I2C_CR1_STOP;
    return TRUE;
}

uint8_t write_i2c(uint8_t *data, uint8_t nbytes){
    return write_i2cs(data, nbytes, 1);
}

/**
 * read nbytes of data from I2C line
 * `data` should be an array with at least `nbytes` length
 * @return 1 if all OK, 0 if NACK or no device found
 */
static uint8_t read_i2cb(uint8_t *data, uint8_t nbytes, uint8_t wait){
    if(wait) I2C_LINEWAIT();
    I2C1->SR1 = 0; // clear previous NACK flag & other error flags
    if(nbytes == 1) return i2c_7bit_receive_onebyte(data, 1);
    else if(nbytes == 2) return i2c_7bit_receive_twobytes(data);
    I2C1->CR1 |= I2C_CR1_START | I2C_CR1_ACK; // generate start sequence, set pos & ack
    I2C_WAIT(I2C1->SR1 & I2C_SR1_SB);       // wait for SB
    DBG("n got SB\n");
    (void) I2C1->SR1;                       // clear SB
    I2C1->DR = addr7r;                      // set address
    I2C_WAIT(I2C1->SR1 & I2C_SR1_ADDR);     // wait for ADDR flag
    DBG("n send addr\n");
    if(I2C1->SR1 & I2C_SR1_AF){             // NACK
        DBG("n NACKed\n");
        return FALSE;
    }
    DBG("n ACKed\n");
    (void) I2C1->SR2;                       // clear ADDR
    for(uint16_t x = nbytes - 3; x > 0; --x){
        I2C_WAIT(I2C1->SR1 & I2C_SR1_RXNE); // wait next byte
        *data++ = I2C1->DR;                 // get data
    }
    DBG("n three left\n");
    // three bytes remain to be read
    I2C_WAIT(I2C1->SR1 & I2C_SR1_RXNE);     // wait dataN-2
    DBG("n dataN-2\n");
    I2C_WAIT(I2C1->SR1 & I2C_SR1_BTF);      // wait for BTF
    DBG("n BTF\n");
    I2C1->CR1 &= ~I2C_CR1_ACK;              // clear ACK
    *data++ = I2C1->DR;                     // read dataN-2
    I2C1->CR1 |= I2C_CR1_STOP;              // program STOP
    *data++ = I2C1->DR;                     // read dataN-1
    I2C_WAIT(I2C1->SR1 & I2C_SR1_RXNE);     // wait next byte
    *data = I2C1->DR;                       // read dataN
    DBG("n got it\n");
    return TRUE;
 }

uint8_t read_i2c(uint8_t *data, uint8_t nbytes){
    return read_i2cb(data, nbytes, 1);
}

// read register reg
uint8_t read_i2c_reg(uint8_t reg, uint8_t *data, uint8_t nbytes){
    if(nbytes == 0) return write_i2cs(&reg, 1, 1);
    DBG("wrote address, now read data\n");
    if(!write_i2cs(&reg, 1, 0)) return FALSE;
    return read_i2cb(data, nbytes, 0);
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
    i2c_set_addr7(i2caddr++);
    if(!read_i2c_reg(0, NULL, 0)) return FALSE;
    return TRUE;
}
