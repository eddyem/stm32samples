/*
 * This file is part of the cordic project.
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

static void timer_setup(){
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
    __DSB();
    TIM2->PSC = 84;             // 85 MHz / 85 = 1 MHz
    TIM2->ARR = 0xFFFFFFFF;     // 32-bit auto-reload (maximum)
    TIM2->CR1 = 0;              // disable counter
}

void timer_start(){
    TIM2->CNT = 0;
    TIM2->CR1 |= TIM_CR1_CEN;
}

void timer_stop(){
    TIM2->CR1 = 0;
}

uint32_t timer_read(){
    return TIM2->CNT;
}

void gpio_setup(){
    RCC->AHB2ENR = RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN;
    __DSB();
    // PC6 as output, PC13 as input with pulldown
    GPIOC->MODER = (0xffffffff & ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE13)) | MODER_O(6) | MODER_I(13);
    GPIOC->PUPDR = PUPD_PD(13);
    GPIOC->OTYPER = OTYPER_PP(6);
    // PA9 (Tx), PA10 (Rx) as alternate functions (for USART1)
    GPIOA->MODER = (0xabffffff & ~(GPIO_MODER_MODE9 | GPIO_MODER_MODE10)) |
                   MODER_AF(9) | MODER_AF(10);
    GPIOA->AFR[1] = AFRf(7, 9) | AFRf(7, 10);
    // count milliseconds
    SysTick_Config(SysFreq / 1000);

    // Setup TIM2 for microsecond counting
    timer_setup();
}


