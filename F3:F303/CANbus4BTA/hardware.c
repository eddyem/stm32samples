/*
 * This file is part of the canbus4bta project.
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

#include "adc.h"
#include "hardware.h"
#include "usart.h"

static inline void gpio_setup(){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN;
    // PWM - AF1 @PA7; USB - alternate function 14 @ pins PA11/PA12; USART1 = AF7 @PA9/10; SWD - AF0 @PA13/14
    GPIOA->AFR[0] = AFRf(1, 7);
    GPIOA->AFR[1] = AFRf(7, 9) | AFRf(7, 10) | AFRf(14, 11) | AFRf(14, 12);
    // USART1: PA10(Rx), PA9(Tx); USB - PA11, PA12; SWDIO - PA13, PA14;
    GPIOA->MODER = MODER_AI(0)  | MODER_AI(1)  | MODER_AI(2)  | MODER_AI(3)  | MODER_I(4)   | MODER_O(5)   |
                   MODER_O(6)   | MODER_AF(7)  | MODER_I(8)   | MODER_AF(9)  | MODER_AF(10) | MODER_AF(11) |
                   MODER_AF(12) | MODER_AF(13) | MODER_AF(14) | MODER_I(15);
    GPIOA->OSPEEDR = OSPEED_MED(7) | OSPEED_MED(9) | OSPEED_MED(10) | OSPEED_HI(11) | OSPEED_HI(12) | OSPEED_HI(13) | OSPEED_HI(14);
    // SPI for SSI: AF5 @PB3, PB4; I2C1: AF4 @PB6, PB7; CAN: AF9 @PB8, PB9; SPI2: AF5 @PB12..PB15
    GPIOB->AFR[0] = AFRf(5, 3) | AFRf(5, 4) | AFRf(4, 6) | AFRf(4, 7);
    GPIOB->AFR[1] = AFRf(9, 8) | AFRf(9, 9) | AFRf(5, 12) | AFRf(5, 13) | AFRf(5, 14) | AFRf(5, 15);
    GPIOB->MODER = MODER_O(0)   | MODER_O(1)   | MODER_O(2)  | MODER_AF(3) | MODER_AF(4)  | MODER_AF(6)  | MODER_AF(7) |
                   MODER_AF(8)  | MODER_AF(9)  | MODER_I(10) | MODER_I(11) | MODER_AF(12) | MODER_AF(13) |
                   MODER_AF(14) | MODER_AF(15);
    GPIOB->OSPEEDR = OSPEED_HI(3)  | OSPEED_HI(4)  | OSPEED_HI(6)  | OSPEED_HI(7)  | OSPEED_HI(8) | OSPEED_HI(9) |
                     OSPEED_HI(12) | OSPEED_HI(13) | OSPEED_HI(14) | OSPEED_HI(15);
    GPIOC->MODER = MODER_O(13) | MODER_O(14) | MODER_O(15);
}

void hw_setup(){
    gpio_setup();
    adc_setup();
}

