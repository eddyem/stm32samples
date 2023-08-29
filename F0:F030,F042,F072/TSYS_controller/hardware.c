/*
 *                                                                                                  geany_encoding=koi8-r
 * hardware.c - hardware-dependent macros & functions
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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

I2C_SPEED curI2Cspeed = LOW_SPEED;


void gpio_setup(void){
    // here we turn on clocking for all periph.
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOAEN | RCC_AHBENR_DMAEN;
    // Set LEDS (PB10/11) & multiplexer as output (PB0..2,12)
    GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODER10  | GPIO_MODER_MODER11 |
                                     GPIO_MODER_MODER0   | GPIO_MODER_MODER1  |
                                     GPIO_MODER_MODER2   | GPIO_MODER_MODER12 )
                    ) |
                    GPIO_MODER_MODER10_O | GPIO_MODER_MODER11_O |
                    GPIO_MODER_MODER0_O  | GPIO_MODER_MODER1_O  |
                    GPIO_MODER_MODER2_O  | GPIO_MODER_MODER12_O;
    // multiplexer outputs are push-pull, GPIOB->OTYPER = 0
    MUL_OFF();
    // PA8 - power enable
    GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER8)) |
                    GPIO_MODER_MODER8_O;
    // PA13..15 (low bits) + PB15 (high bit) - CAN address, pullup inputs
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(GPIO_PUPDR_PUPDR13 | GPIO_PUPDR_PUPDR14 |
                                     GPIO_PUPDR_PUPDR15)
                    ) |
                    GPIO_PUPDR_PUPDR13_0 | GPIO_PUPDR_PUPDR14_0 |
                    GPIO_PUPDR_PUPDR15_0;
    GPIOB->PUPDR = (GPIOB->PUPDR & ~(GPIO_PUPDR_PUPDR15)) |
                   GPIO_PUPDR_PUPDR15_0;
    pin_set(LED0_port, LED0_pin); // clear LEDs
    pin_set(LED1_port, LED1_pin);
}

void i2c_setup(I2C_SPEED speed){
    if(speed == CURRENT_SPEED){
        speed = curI2Cspeed;
    }else{
        curI2Cspeed = speed;
    }
    I2C1->CR1 = 0;
#if I2CPINS == 910
/*
 * GPIO Resources: I2C1_SCL - PA9, I2C1_SDA - PA10
 * GPIOA->AFR[1]
 */
    GPIOA->AFR[1] &= ~0xff0; // alternate function F4 for PA9/PA10
    GPIOA->AFR[1] |= 0x440;
    GPIOA->MODER &= ~(GPIO_MODER_MODER9 | GPIO_MODER_MODER10);
    GPIOA->MODER |= GPIO_MODER_MODER9_AF | GPIO_MODER_MODER10_AF; // alternate function
    GPIOA->OTYPER |= GPIO_OTYPER_OT_9 | GPIO_OTYPER_OT_10; // opendrain
    //GPIOA->OTYPER |= GPIO_OTYPER_OT_10; // opendrain
#elif I2CPINS == 67
/*
 * GPIO Resources: I2C1_SCL - PB6, I2C1_SDA - PB7 (AF1)
 * GPIOB->AFR[0] ->   1<<6*4 | 1<<7*4 = 0x11000000
 */
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~0xff000000) | 0x11000000;
    GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODER6 | GPIO_MODER_MODER7)) |
                    GPIO_MODER_MODER6_AF | GPIO_MODER_MODER7_AF;
    GPIOB->OTYPER |= GPIO_OTYPER_OT_6 | GPIO_OTYPER_OT_7;
#else // undefined
#error "Not implemented"
#endif
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
    I2C1->CR1 = I2C_CR1_PE;// | I2C_CR1_RXIE; // Enable I2C & (interrupt on receive - not supported yet)
}
