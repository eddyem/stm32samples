/*
 * This file is part of the SevenCDCs project.
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
#include "usart.h"

TRUE_INLINE void iwdg_setup(){
    uint32_t tmout = 16000000;
    /* Enable the peripheral clock RTC */
    /* (1) Enable the LSI (40kHz) */
    /* (2) Wait while it is not ready */
    RCC->CSR |= RCC_CSR_LSION; /* (1) */
    while((RCC->CSR & RCC_CSR_LSIRDY) != RCC_CSR_LSIRDY){if(--tmout == 0) break;} /* (2) */
    /* Configure IWDG */
    /* (1) Activate IWDG (not needed if done in option bytes) */
    /* (2) Enable write access to IWDG registers */
    /* (3) Set prescaler by 64 (1.6ms for each tick) */
    /* (4) Set reload value to have a rollover each 2s */
    /* (5) Check if flags are reset */
    /* (6) Refresh counter */
    IWDG->KR = IWDG_START; /* (1) */
    IWDG->KR = IWDG_WRITE_ACCESS; /* (2) */
    IWDG->PR = IWDG_PR_PR_1; /* (3) */
    IWDG->RLR = 1250; /* (4) */
    tmout = 16000000;
    while(IWDG->SR){if(--tmout == 0) break;} /* (5) */
    IWDG->KR = IWDG_REFRESH; /* (6) */
}


static inline void gpio_setup(){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
    // USB - alternate function 14 @ pins PA11/PA12; USART1 = AF7 @PA9/10; SWD - AF0 @PA13/14
    GPIOA->AFR[1] = AFRf(7, 9) | AFRf(7, 10) | AFRf(14, 11) | AFRf(14, 12);
    // USART1: PA10(Rx), PA9(Tx); USB - PA11, PA12; SWDIO - PA13, PA14; Pullup - PA15
    GPIOA->MODER = MODER_AF(9) | MODER_AF(10) | MODER_AF(11) | MODER_AF(12) | MODER_AF(13) | MODER_AF(14) | MODER_O(15);
    GPIOA->OSPEEDR = OSPEED_HI(11) | OSPEED_HI(12) | OSPEED_HI(13) | OSPEED_HI(14);
    // LEDs on PB0 and PB1
    GPIOB->MODER = GPIO_MODER_MODER0_O | GPIO_MODER_MODER1_O;
    GPIOB->ODR = 1;
}

void hw_setup(){
    gpio_setup();
    iwdg_setup();
}

