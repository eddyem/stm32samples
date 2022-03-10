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
 * I2C for HTU21D
 * Speed <= 400kHz (200)
 * t_SCLH ~ 0.6us
 * t_SCLL ~ 1.3us
 * t_SU >= 100ns
 * t_HD <= 900ns
 * t_VD <= 400ns
 *
 * After start 15ms pause for IDLE
 * Start bit: SCK=1, DATA 1->0
 * Stop bit: SCK 0->1, DATA 0->1
 * Address: 0x40 (7bit), 0th bit - direction (0 - write, 1 - read)
 * ACK: DATA->0 on 8th SCK clock
 * Commands: 0xE3[F3] - temperature, 0xE5[F5] - humidity [No hold master]
 *           0xE6 - write user register, 0xE7 - read user register, 0xFE - soft reset
 *                 7    6   5 4 3    2   1    0
 * User register: |D1|Vbat|reserved|Htr|Odis|D0|  default: 0x02
 * D1D0 - resolution [H/T]: 00-12/14, 01-8/12, 10-10/13, 11-11/11
 * Vbat=1 vhen Vdd<2.25V, Htr=1 to enable on-chip heater,
 * Odis=0 to enable OTP reload (after each measurement reload defaults)
 *
 * in = in & 0xFFFC;
 * Calculations: RH = -6 + 125*Hum/2^16
 *                T = -46.85 + 175.72*Temp/2^16
 */

/*
 * Resources: I2C1_SCL - PA9, I2C1_SDA - PA10
 * GPIOA->AFR[1] AF4 -- GPIOA->AFR[1] &= ~0xff0, GPIOA->AFR[1] |= 0x440
 */
extern volatile uint32_t Tms;
static uint32_t cntr;

void i2c_setup(){
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
// I2C2->TIMINGR = (uint32_t)0x00B01A4B;
// PRECS=0 - tpresc=20.8(3)ns, SCLDEL=B (tSU 12tpresc), SDADEL=0 (tHD=0), SCLH=1a (27), SCLL=4B (76)
// https://github.com/mattbrejza/magnet-node/blob/master/firmware-basestation/htu21.c
// prescaler = 1, low period = 0x13, high period = 0xf, hold time = 2, setup = 4
// I2C_TIMINGR(H_I2C) = (1<<28) | (4<<20) | (2<<16) | (0xf<<8) | 0x13;

/* transmit:
 * I2Cx_CR2:
 *      Addressing mode (7-bit or 10-bit): ADD10
 *      Slave address to be sent: SADD[9:0]
 *      Transfer direction: I2C_CR2_RD_WRN=0
 *      The number of bytes to be transferred: NBYTES[7:0] (NBYTES<<16)
 *      You must then set the START bit in I2Cx_CR2 register.
 *      Automatic end mode (AUTOEND = '1' in the I2Cx_CR2 register),
 * I2C1->CR2 = I2C_CR2_HEAD10R | SLAVE_ADDR; // 7bit, slave address
 */
// return 1 if all OK, 0 if NACK
uint8_t htu_write_i2c(uint8_t data){
    cntr = Tms;
    while(I2C1->ISR & I2C_ISR_BUSY) if(Tms - cntr > 5) return 0;  // check busy
    cntr = Tms;
    while(I2C1->CR2 & I2C_CR2_START) if(Tms - cntr > 5) return 0; // check start
    I2C1->CR2 = 1<<16 | HTU21_ADDR | I2C_CR2_AUTOEND;  // 1 byte, autoend
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

#define SHIFTED_DIVISOR 0x988000    //This is the 0x0131 polynomial shifted to farthest left of three bytes
// check CRC, return 0 if all OK
uint32_t htu_check_crc(uint16_t data, uint8_t crc){
    uint32_t remainder = (uint32_t)data << 8;
    remainder |= crc;
    uint32_t divsor = (uint32_t)SHIFTED_DIVISOR;
    int i;
    for(i = 0; i < 16; i++) {
        if (remainder & (uint32_t)1 << (23 - i))
            remainder ^= divsor;
        divsor >>= 1;
    }
    return remainder;
}

// return 1 if all OK, 0 if NACK
uint8_t htu_read_i2c(uint16_t *data){
    uint8_t buf[3];
    cntr = Tms;
    while(I2C1->ISR & I2C_ISR_BUSY) if(Tms - cntr > 5) return 0;  // check busy
    cntr = Tms;
    while(I2C1->CR2 & I2C_CR2_START) if(Tms - cntr > 5) return 0; // check start
    // read three bytes
    I2C1->CR2 = 3<<16 | HTU21_ADDR | 1 | I2C_CR2_AUTOEND | I2C_CR2_RD_WRN;
    I2C1->CR2 |= I2C_CR2_START;
    int i;
    cntr = Tms;
    for(i = 0; i < 3; ++i){
        while(!(I2C1->ISR & I2C_ISR_RXNE)){ // wait for data
            if(I2C1->ISR & I2C_ISR_NACKF){
                I2C1->ICR |= I2C_ICR_NACKCF;
                return 0;
            }
            if(Tms - cntr > 5) return 0;
        }
        buf[i] = I2C1->RXDR;
    }
    *data = (buf[0] << 8) | buf[1];
    if(htu_check_crc(*data, buf[2])) return 1; // CRC error
    return 1;
 }

// return 0 if all OK
uint8_t htu_read_reg(uint8_t *data){
    cntr = Tms;
    while(I2C1->ISR & I2C_ISR_BUSY) if(Tms - cntr > 5) return 0;  // check busy
    cntr = Tms;
    while(I2C1->CR2 & I2C_CR2_START) if(Tms - cntr > 5) return 0; // check start
    I2C1->CR2 = 1<<16 | HTU21_ADDR;  // 1 byte
    I2C1->CR2 |= I2C_CR2_START;
    cntr = Tms;
    while(!(I2C1->ISR & I2C_ISR_TXIS)){ // ready to transmit
        if(I2C1->ISR & I2C_ISR_NACKF){
            I2C1->ICR |= I2C_ICR_NACKCF;
            return 0;
        }
        if(Tms - cntr > 5) return 0;
    }
    I2C1->TXDR = HTU21_READ_REG;
    cntr = Tms;
    while(!(I2C1->ISR & I2C_ISR_TC)) if(Tms - cntr > 5) return 0; // wait transfer completion
    I2C1->CR2 = 1<<16 | HTU21_ADDR | 1 | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND; // receive, autoend
    I2C1->CR2 |= I2C_CR2_START;
    cntr = Tms;
    while(!(I2C1->ISR & I2C_ISR_RXNE)){ // wait for data
        if(I2C1->ISR & I2C_ISR_NACKF){
            I2C1->ICR |= I2C_ICR_NACKCF;
            return 0;
        }
        if(Tms - cntr > 5) return 0;
    }
    *data = I2C1->RXDR;
    return 1;
}

uint8_t htu_write_reg(uint8_t data){
    cntr = Tms;
    while(I2C1->ISR & I2C_ISR_BUSY) if(Tms - cntr > 5) return 0;  // check busy
    cntr = Tms;
    while(I2C1->CR2 & I2C_CR2_START) if(Tms - cntr > 5) return 0; // check start
    I2C1->CR2 = 1<<16 | HTU21_ADDR;  // 1 byte
    I2C1->CR2 |= I2C_CR2_START;
    cntr = Tms;
    while(!(I2C1->ISR & I2C_ISR_TXIS)){ // ready to transmit
        if(I2C1->ISR & I2C_ISR_NACKF){
            I2C1->ICR |= I2C_ICR_NACKCF;
            return 0;
        }
        if(Tms - cntr > 5) return 0;
    }
    I2C1->TXDR = HTU21_WRITE_REG;
    cntr = Tms;
    while(!(I2C1->ISR & I2C_ISR_TC)) if(Tms - cntr > 5) return 0; // wait transfer completion
    I2C1->CR2 |= I2C_CR2_AUTOEND; // receive, autoend
    I2C1->CR2 |= I2C_CR2_START;
    cntr = Tms;
    while(!(I2C1->ISR & I2C_ISR_TXIS)){ // ready to transmit
        if(I2C1->ISR & I2C_ISR_NACKF){
            I2C1->ICR |= I2C_ICR_NACKCF;
            return 0;
        }
        if(Tms - cntr > 5) return 0;
    }
    I2C1->TXDR = data;
    return 1;
}


//output in Cx10
int16_t convert_temperature(uint16_t in){
    in = in & 0xFFFC;
    uint32_t a = (uint32_t)in * 17572;
    a >>= 16;
    int16_t val = ((int16_t)a - 4685)/10;
    return val;
}

//output in %x10
int16_t convert_humidity(uint16_t in){
    in = in & 0xFFFC;
    uint32_t a = (uint32_t)in * 1250;
    a >>= 16;
    int16_t val = (int16_t)a - 60;
    return val;
}
