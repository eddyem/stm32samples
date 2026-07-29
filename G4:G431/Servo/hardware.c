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

#include "flash.h"
#include "hardware.h"
#include "servo.h"

volatile uint32_t Tms = 0;
volatile bool tim_triggered[SERVO_AMOUNT];

/* Called when systick fires */
void sys_tick_handler(){
    ++Tms;
}

// try to set servo; if wrong return false
bool set_servo(uint8_t N, uint16_t val){
    if(N >= SERVO_AMOUNT) return false;
    if(val < the_conf.minpulse[N] || val > the_conf.maxpulse[N]) return false;
    volatile uint32_t *CCR = &(TIM3->CCR1);
    CCR[N] = val;
    return true;
}

bool get_servo(uint8_t N, uint16_t *val){
    if(N >= SERVO_AMOUNT) return false;
    volatile uint32_t *CCR = &(TIM3->CCR1);
    if(val) *val = CCR[N];
    return true;
}

static void tim3_setup(){
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;
    __DSB();
    // PWM mode 1 (active -> inactive) on all three channels; preload enabled
    TIM3->CCMR1 =   TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 |
                    TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 |
                    TIM_CCMR1_OC1PE | TIM_CCMR1_OC2PE;
    TIM3->CCMR2 =   TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1 |
                    TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1 |
                    TIM_CCMR2_OC3PE | TIM_CCMR2_OC4PE;
    // frequency
    TIM3->PSC = 84; // 1MHz -> 1us per tick
    // ARR for PWM
    TIM3->ARR = 19999; // 20ms, 50Hz
    // CCRx - minimal position
    TIM3->CCR1 = the_conf.startpulse[0];
    TIM3->CCR2 = the_conf.startpulse[1];
    TIM3->CCR3 = the_conf.startpulse[2];
    TIM3->CCR4 = the_conf.startpulse[3];
    // enable CCx interrupts
    TIM3->DIER = TIM_DIER_CC1IE | TIM_DIER_CC2IE | TIM_DIER_CC3IE | TIM_DIER_CC4IE;
    // enable main output
    //TIM3->BDTR |= TIM_BDTR_MOE;
    // enable PWM output
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E |TIM_CCER_CC3E | TIM_CCER_CC4E;
    // enable timer & ARR buffering
    TIM3->CR1 |= TIM_CR1_CEN | TIM_CR1_ARPE;
    // update buffers
    TIM3->EGR = TIM_EGR_UG;
    // and enable interrupt
    NVIC_EnableIRQ(TIM3_IRQn);
}

// TIM3 channels: 1 - PA6 (AF2), 2 - PA7 (AF2), 3 - PB0 (AF2), 4 - PB1 (AF2)
void gpio_setup(){
    RCC->AHB2ENR = RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;
    __DSB(); // Data Synchronization Barrier - wait until data will be really written to AHB2ENR
    // set PC6 as push-pull output, PC13 is pulldown input, other as default (AIN)
    GPIOC->MODER = (0xffffffff & ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE13)) | MODER_O(6) | MODER_I(13);
    GPIOC->PUPDR = PUPD_PD(13); // pulldown
    GPIOC->OTYPER = OTYPER_PP(6); // push-pull (default)
    // PA9 (Tx) and PA10 (Rx) (don't forget about SWDIO)
    GPIOA->MODER = (0xabffffff & ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE7 | GPIO_MODER_MODE9 | GPIO_MODER_MODE10)) |
                   MODER_AF(6) | MODER_AF(7) | MODER_AF(9) | MODER_AF(10);
    GPIOA->AFR[0] = AFRf(2, 6) | AFRf(2, 7);
    GPIOA->AFR[1] = AFRf(7, 9) | AFRf(7, 10); // SWDIO is 0 by default; USART1 is AF7
    GPIOB->MODER = (0xffffffff & ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE1)) |
                   MODER_AF(0) | MODER_AF(1);
    GPIOB->AFR[0] = AFRf(2, 0) | AFRf(2, 1);
    // count milliseconds
    SysTick_Config(SysFreq / 1000); // arg should be < 0xffffff
    tim3_setup();
}

// update event flags
void tim3_isr(){
    if(TIM3->SR & TIM_SR_CC1IF) tim_triggered[0] = true;
    if(TIM3->SR & TIM_SR_CC2IF) tim_triggered[1] = true;
    if(TIM3->SR & TIM_SR_CC3IF) tim_triggered[2] = true;
    if(TIM3->SR & TIM_SR_CC4IF) tim_triggered[3] = true;
    TIM3->SR = 0;
}
