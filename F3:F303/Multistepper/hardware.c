/*
 * This file is part of the multistepper project.
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

// Buttons: PA9, PA10, PF6, PD3, PD4, PD5, pullup (active - 0)
volatile GPIO_TypeDef* const BTNports[BTNSNO] = {GPIOA, GPIOA, GPIOF, GPIOD, GPIOD, GPIOD};
const uint32_t BTNpins[BTNSNO] = {1<<9, 1<<10, 1<<6, 1<<3, 1<<4, 1<<5};

// setup here ALL GPIO pins (due to table in Readme.md)
// leave SWD as default AF; high speed for CLK and some other AF; med speed for some another AF
TRUE_INLINE void gpio_setup(){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIODEN
                | RCC_AHBENR_GPIOEEN | RCC_AHBENR_GPIOFEN;
    for(int i = 0; i < 10000; ++i) nop();
    GPIOA->ODR = 0;
    GPIOA->AFR[0] = AFRf(5, 5) | AFRf(5, 6) | AFRf(5, 7);
    GPIOA->AFR[1] = /*AFRf(4, 10) |*/ /*AFRf(14, 11) | AFRf(14,12) |*/ AFRf(1, 15);
    GPIOA->MODER = MODER_AF(5) | MODER_AF(6) | MODER_AF(7) | MODER_O(8) | MODER_I(9) | MODER_I(10) /*MODER_AF(10)*/
                 | /*MODER_AF(11) | MODER_AF(12) |*/ MODER_AF(13) | MODER_AF(14) | MODER_AF(15);
    GPIOA->OSPEEDR = OSPEED_MED(5) | OSPEED_MED(6) | OSPEED_MED(7) | OSPEED_HI(11) | OSPEED_HI(12)
                 | OSPEED_HI(13) | OSPEED_HI(15);
    GPIOA->OTYPER = 0;
    GPIOA->PUPDR = PUPD_PU(8) | PUPD_PU(9) | PUPD_PU(10);

    GPIOB->ODR = 0;
    GPIOB->AFR[0] = AFRf(2, 0) | AFRf(7, 3) | AFRf(10, 5);
    GPIOB->AFR[1] = AFRf(1, 8) | AFRf(7, 10) | AFRf(5, 13) | AFRf(5, 14) | AFRf(5, 15);
    GPIOB->MODER = MODER_AF(0) | MODER_O(1) | MODER_O(2) | MODER_AF(3) | MODER_O(4) | MODER_AF(5) | MODER_O(6)
                 | MODER_I(7) | MODER_AF(8) | MODER_I(9) | MODER_AF(10) | MODER_I(11) | MODER_O(12) | MODER_AF(13)
                 | MODER_AF(14) | MODER_AF(15);
    GPIOB->OSPEEDR = OSPEED_HI(0) | OSPEED_HI(5) | OSPEED_HI(8) | OSPEED_MED(13) | OSPEED_MED(14) | OSPEED_MED(15);
    GPIOB->OTYPER = 0;
    GPIOB->PUPDR = PUPD_PU(7) | PUPD_PU(9) | PUPD_PU(11);

    GPIOC->ODR = 0;
    GPIOC->AFR[0] = AFRf(7, 4) | AFRf(7, 5) | AFRf(4, 6);
    GPIOC->MODER = MODER_O(0) | MODER_O(1) | MODER_O(2) | MODER_O(3) | MODER_AF(4) | MODER_AF(5) | MODER_AF(6)
                 | MODER_I(7) | MODER_O(8) | MODER_O(9) | MODER_I(10) | MODER_I(11) | MODER_O(12) | MODER_I(13)
                 | MODER_I(14) | MODER_O(15);
    GPIOC->OSPEEDR = OSPEED_HI(6);
    GPIOC->OTYPER = 0;
    GPIOC->PUPDR = PUPD_PU(7) | PUPD_PU(10) | PUPD_PU(11) | PUPD_PU(13) | PUPD_PU(14);

    GPIOD->ODR = 0;
    GPIOD->AFR[0] = AFRf(7, 0) | AFRf(7, 1);
    GPIOD->AFR[1] = AFRf(2, 12);
    GPIOD->MODER = MODER_AF(0) | MODER_AF(1) | MODER_O(2) | MODER_I(3) | MODER_I(4) | MODER_I(5) | MODER_I(6)
                 | MODER_I(7) | MODER_O(8) | MODER_O(9) | MODER_I(10) | MODER_I(11) | MODER_AF(12) | MODER_O(13)
                 | MODER_O(14) | MODER_I(15);
    GPIOD->OSPEEDR = OSPEED_HI(0) | OSPEED_HI(1) | OSPEED_HI(12);
    GPIOD->OTYPER = 0;
    GPIOD->PUPDR = PUPD_PU(3) | PUPD_PU(4) | PUPD_PU(5) | PUPD_PU(6) | PUPD_PU(7) | PUPD_PU(10) | PUPD_PU(11)
                 | PUPD_PU(15);

    GPIOE->ODR = 0;
    GPIOE->AFR[1] = AFRf(2, 9);
    GPIOE->MODER = MODER_O(0) | MODER_O(1) | MODER_I(2) | MODER_O(3) | MODER_O(4) | MODER_O(5) | MODER_O(6)
                 | MODER_I(7) | MODER_I(8) | MODER_AF(9) | MODER_I(10) | MODER_I(11) | MODER_O(12) | MODER_O(13)
                 | MODER_I(14) | MODER_I(15);
    GPIOE->OSPEEDR = OSPEED_HI(9);
    GPIOE->OTYPER = 0;
    GPIOE->PUPDR = PUPD_PU(2) | PUPD_PU(7) | PUPD_PU(8) | PUPD_PU(10) | PUPD_PU(11) | PUPD_PU(14) | PUPD_PU(15);

    GPIOF->ODR = 0;
    /*GPIOF->AFR[0] = AFRf(4, 6);*/
    GPIOF->AFR[1] = AFRf(3, 9);
    GPIOF->MODER = MODER_I(6) | MODER_AF(9) | MODER_O(10);
    GPIOF->OSPEEDR = OSPEED_HI(9);
    GPIOF->OTYPER = 0;
    GPIOF->PUPDR =  PUPD_PU(6);
}

#ifndef EBUG
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
#endif

void hw_setup(){
    gpio_setup();
#ifndef EBUG
    iwdg_setup();
#endif
}

