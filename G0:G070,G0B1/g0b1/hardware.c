/*
 * This file is part of the test project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

void gpio_setup(){
    RCC->IOPENR = RCC_IOPENR_GPIOCEN | RCC_IOPENR_GPIOBEN;
    // set PC8 as opendrain output, PC0 is pullup input, other as default (AIN)
    GPIOC->MODER = (0xffffffff & ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE13)) | GPIO_MODER_MODER6_O; // GPIO_MODER_MODER13_I == 0
    GPIOC->PUPDR = GPIO_PUPDR13_PD; // pull down
    // USART1: PB6 - Tx (AF0), PB7 - Rx (AF0)
    GPIOB->MODER = MODER_AF(6) | MODER_AF(7);
    GPIOB->AFR[0] = 0;
    // RCC->CCIPR = 0; // default -> sysclk/pclk source
}
