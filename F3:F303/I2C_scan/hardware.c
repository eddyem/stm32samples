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

#include "hardware.h"

static inline void gpio_setup(){
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN; // for LEDs
    for(int i = 0; i < 10000; ++i) nop();
    GPIOB->MODER = GPIO_MODER_MODER0_O | GPIO_MODER_MODER1_O;
    GPIOB->ODR = 1;
}

void hw_setup(){
    gpio_setup();
}

