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
#include "usbhw.h"

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

// setup TIM2 as 24-bit 2mks timestamp counter
TRUE_INLINE void tim2_setup(){
    TIM2->CR1 = 0; // turn off timer
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // enable clocking
    TIM2->PSC = 143; // 500kHz
    TIM2->ARR = 0xffffff; // 24 bit counter from 0xffffff to 0
    // turn it on, downcounting
    TIM2->CR1 = TIM_CR1_CEN | TIM_CR1_DIR;
}

TRUE_INLINE void gpio_setup(){
    RELAY_OFF(); MUL_OFF(0); MUL_OFF(1);
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN;
    // PWM - AF1 @PA7; USB - alternate function 14 @ pins PA11/PA12; SWD - AF0 @PA13/14
    GPIOA->AFR[0] = AFRf(1, 7);
    GPIOA->AFR[1] = AFRf(14, 11) | AFRf(14, 12);
    // PA4 - din (PU in); USB - PA11, PA12; SWDIO - PA13, PA14; PA8 & PA15 - PU in
    GPIOA->PUPDR = (GPIOA->PUPDR & PUPD_CLR(4) & PUPD_CLR(8) & PUPD_CLR(10) & PUPD_CLR(15)) |
                   PUPD_PU(4) | PUPD_PU(8) | PUPD_PU(10) | PUPD_PU(15);
    GPIOA->MODER = MODER_AI(0)  | MODER_AI(1)  | MODER_AI(2)  | MODER_AI(3)  | MODER_I(4)   | MODER_O(5)   |
                   MODER_O(6)   | MODER_AF(7)  | MODER_I(8)   | MODER_AF(11) |
                   MODER_AF(12) | MODER_AF(13) | MODER_AF(14) | MODER_I(15);
    GPIOA->OSPEEDR = OSPEED_HI(4) | OSPEED_MED(7) | OSPEED_MED(9) | OSPEED_MED(10) | OSPEED_HI(11) |
                     OSPEED_HI(12) | OSPEED_HI(13) | OSPEED_HI(14);
    // I2C1: AF4 @PB6, PB7; CAN: AF9 @PB8, PB9; SPI2: AF5 @PB12..PB15
    GPIOB->AFR[0] = AFRf(5, 3) | AFRf(5, 4) | AFRf(4, 6) | AFRf(4, 7);
    GPIOB->AFR[1] = AFRf(9, 8) | AFRf(9, 9);
    // PB10,11 - PU in
    GPIOB->PUPDR = PUPD_PU(10) | PUPD_PU(11);
    GPIOB->MODER = MODER_O(0)   | MODER_O(1)   | MODER_O(2)  | MODER_AF(6)  | MODER_AF(7) |
                   MODER_AF(8)  | MODER_AF(9)  | MODER_I(10) | MODER_I(11);
    GPIOB->OSPEEDR = OSPEED_HI(3)  | OSPEED_HI(4)  | OSPEED_HI(6)  | OSPEED_HI(7)  | OSPEED_HI(8) | OSPEED_HI(9) |
                     OSPEED_HI(12) | OSPEED_HI(13) | OSPEED_HI(14) | OSPEED_HI(15);
    GPIOC->MODER = MODER_O(13) | MODER_O(14) | MODER_O(15);
}

void hw_setup(){
    tim2_setup();
    gpio_setup();
    adc_setup();
    USB_setup();
#ifndef EBUG
    iwdg_setup();
#endif
}

