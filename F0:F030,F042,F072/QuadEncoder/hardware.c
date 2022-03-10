/*
 * This file is part of the Chiller project.
 * Copyright 2018 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

volatile uint8_t tim3upd = 0;

/**
 * @brief gpio_setup - setup GPIOs for external IO
 * GPIO pinout:
 *      PA4 - open drain        - onboard LED (always ON when board works)
 *      PA6, PA7 - TIM3_CH1/CH2 - encoder input
 */
static inline void gpio_setup(){
    // Enable clocks to the GPIO subsystems
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOFEN;
    // PA6/7 - AF; PB1 - AF
    GPIOA->MODER = GPIO_MODER_MODER4_O | GPIO_MODER_MODER6_AF | GPIO_MODER_MODER7_AF;
    GPIOA->OTYPER = GPIO_OTYPER_OT_4;
    // alternate functions:
    // PA6 - TIM3_CH1, PA7 - TIM3_CH2
    GPIOA->AFR[0] = (GPIOA->AFR[0] &~ (GPIO_AFRL_AFRL6 | GPIO_AFRL_AFRL7)) \
                | (1 << (6 * 4)) | (1 << (7 * 4));
}

static inline void timers_setup(){
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    /* (1) Configure TI1FP1 on TI1 (CC1S = 01)
           configure TI1FP2 on TI2 (CC2S = 01) */
    /* (2) Configure TI1FP1 and TI1FP2 non inverted (CC1P = CC2P = 0, reset value) */
    /* (3) Configure both inputs are active on both rising and falling edges
          (SMS = 011), set external trigger filter to f_DTS/8, N=6 (ETF=1000) */
    /* (4) Enable the counter by writing CEN=1 in the TIMx_CR1 register. */
    TIM3->CCMR1 = TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0; /* (1)*/
    //TIMx->CCER &= (uint16_t)(~(TIM_CCER_CC21 | TIM_CCER_CC2P); /* (2) */
    TIM3->SMCR =  TIM_SMCR_ETF_3 | TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1; /* (3) */
    // enable update interrupt
    TIM3->DIER = TIM_DIER_UIE;
    // set ARR to 79 - generate interrupt each 80 counts (one revolution)
    TIM3->ARR = 79;
    // enable timer
    TIM3->CR1 = TIM_CR1_CEN; /* (4) */
    NVIC_EnableIRQ(TIM3_IRQn);
}

void hw_setup(){
    sysreset();
    gpio_setup();
    timers_setup();
    USART1_config();
}

void tim3_isr(){
    if(TIM3->SR & TIM_SR_UIF){
        tim3upd = 1;
    }
    TIM3->SR = 0;
}
