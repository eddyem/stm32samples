/*
 * This file is part of the mlxtest project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

static inline void gpio_setup(){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN; // for USART and LEDs
    for(int i = 0; i < 10000; ++i) nop();
    // USB - alternate function 14 @ pins PA11/PA12; SWD - AF0 @PA13/14
    GPIOA->AFR[1] = AFRf(14, 11) | AFRf(14, 12);
    GPIOA->MODER = MODER_AF(11) | MODER_AF(12) | MODER_AF(13) | MODER_AF(14) | MODER_O(15);
    GPIOB->MODER = MODER_O(0) | MODER_O(1);
    GPIOB->ODR = 1;
}

void hw_setup(){
    gpio_setup();
}

