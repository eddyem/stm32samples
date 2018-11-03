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
#include "usart.h"

static uint8_t brdADDR = 0;

void gpio_setup(void){
    // here we turn on clocking for all periph.
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN | RCC_AHBENR_DMAEN;
    // Set LEDS (PC13/14) as output
    GPIOC->MODER = (GPIOC->MODER & ~(GPIO_MODER_MODER13  | GPIO_MODER_MODER14)
                    ) |
                    GPIO_MODER_MODER13_O | GPIO_MODER_MODER14_O;
    // PB14(0), PB15(1), PA8(2) - board address, pullup inputs
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(GPIO_PUPDR_PUPDR8)
                    ) |
                    GPIO_PUPDR_PUPDR8_0;
    GPIOB->PUPDR = (GPIOB->PUPDR & ~(GPIO_PUPDR_PUPDR14 | GPIO_PUPDR_PUPDR15)
                    ) |
                    GPIO_PUPDR_PUPDR14_0 | GPIO_PUPDR_PUPDR15_0;
    pin_set(LED0_port, LED0_pin); // clear LEDs
    pin_set(LED1_port, LED1_pin);
    uint8_t addr = READ_BRD_INV_ADDR();
    brdADDR = ~addr & 0x7;
}

uint8_t getBRDaddr(){return brdADDR;}
