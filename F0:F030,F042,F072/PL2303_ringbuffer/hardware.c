/*
 * This file is part of the pl2303 project.
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

static inline void gpio_setup(){
    // here we turn on clocking for all periph.
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    LED_off(LED0);
    LED_off(LED1);
    // Set LEDs (PB0/1) as output
    GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1)
                    ) |
                    GPIO_MODER_MODER0_O | GPIO_MODER_MODER1_O;
}

void hw_setup(){
    gpio_setup();
}

