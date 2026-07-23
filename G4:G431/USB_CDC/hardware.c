/*
 * This file is part of the usart project.
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

#include <stm32g4.h>

#include "hardware.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(){
    ++Tms;
}

void gpio_setup(){
    RCC->AHB2ENR = RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN; // enable PC & PA (USART1)
    __DSB(); // Data Synchronization Barrier - wait until data will be really written to AHB2ENR
    // set PC6 as push-pull output, PC13 is pulldown input, other as default (AIN)
    GPIOC->MODER = (0xffffffff & ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE13)) | MODER_O(6) | MODER_I(13);
    GPIOC->PUPDR = PUPD_PD(13); // pulldown
    GPIOC->OTYPER = OTYPER_PP(6); // push-pull (default)
    // PA9 (Tx) and PA10 (Rx) (don't forget about SWDIO)
    GPIOA->MODER = (0xabffffff & ~(GPIO_MODER_MODE9 | GPIO_MODER_MODE10)) | MODER_AF(9) | MODER_AF(10);
    GPIOA->AFR[1] = AFRf(7, 9) | AFRf(7, 10); // SWDIO is 0 by default; USART1 is AF7
    // count milliseconds
    SysTick_Config(SysFreq / 1000); // arg should be < 0xffffff
}

