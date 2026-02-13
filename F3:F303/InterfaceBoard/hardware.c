/*
 * This file is part of the multiiface project.
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

uint8_t Config_mode = 0;

static inline void gpio_setup(){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_DMA1EN | RCC_AHBENR_DMA2EN;
    RCC->APB1ENR |= RCC_APB1ENR_CANEN | RCC_APB1ENR_USART2EN | RCC_APB1ENR_USART3EN
                    | RCC_APB1ENR_UART4EN | RCC_APB1ENR_UART5EN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    for(int i = 0; i < 10000; ++i) nop();
    // USB - alternate function 14 @ pins PA11/PA12; SWD - AF0 @PA13/14
    GPIOA->AFR[1] = AFRf(14, 11) | AFRf(14, 12);
    // PA9 - config jumper (PU in), PA10 - USB pullup (PP)
    GPIOA->MODER = MODER_I(9) | MODER_O(10) | MODER_AF(11) | MODER_AF(12) | MODER_AF(13) | MODER_AF(14);
    GPIOA->OSPEEDR = OSPEED_HI(11) | OSPEED_HI(12) | OSPEED_HI(13) | OSPEED_HI(14);
    GPIOA->PUPDR = PUPD_PU(9);
    for(int i = 0; i < 10000; ++i) nop();
    if(CFG_ON()) Config_mode = 1;
}

void hw_setup(){
    gpio_setup();
}

