/*
 * This file is part of the usbcangpio project.
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

#include <stm32f0.h>
#include <string.h>

#include "pwm.h"

static volatile TIM_TypeDef * const timers[TIMERS_AMOUNT] = {
    [TIM1_IDX]  = TIM1,
    [TIM2_IDX]  = TIM2,
    [TIM3_IDX]  = TIM3,
    [TIM14_IDX] = TIM14,
    [TIM16_IDX] = TIM16,
};

#if 0
PWM (start - collisions):
PxN XY (XY: TIMX_CHY)
PA1  22 *
PA2  23 **
PA3  24 ***
PA6  161
PA7  141
PA9  12
PA10 13
PB0  33
PB1  34
PB3  22 *
PB4  31
PB5  32
PB10 23 **
PB11 24 ***
-> need to set up timers / channels
TIM1 / 2 3
TIM2 / 2 3 4
TIM3 / 1 2 3 4
TIM14 / 1
TIM16 / 1
#endif

#define PT(i, ch)           {.timidx = i, .chidx = ch}
#define PTC(i, ch, P, p)    {.timidx = i, .chidx = ch, .collision = 1, .collport = P, .collpin = p}
static const pwmtimer_t timer_map[2][16] = {
    [0] = {
        [1]  = PTC(TIM2_IDX, 1, 1, 3),
        [2]  = PTC(TIM2_IDX, 2, 1, 10),
        [3]  = PTC(TIM2_IDX, 3, 1, 11),
        [6]  = PT(TIM16_IDX, 0),
        [7]  = PT(TIM14_IDX, 0),
        [9]  = PT(TIM1_IDX, 1),
        [10] = PT(TIM1_IDX, 2)
    },
    [1] = {
        [0]  = PT(TIM3_IDX, 2),
        [1]  = PT(TIM3_IDX, 3),
        [3]  = PTC(TIM2_IDX, 1, 0, 1),
        [4]  = PT(TIM3_IDX, 0),
        [5]  = PT(TIM3_IDX, 1),
        [10] = PTC(TIM2_IDX, 2, 0, 2),
        [11] = PTC(TIM2_IDX, 3, 0, 3)
    }
};
#undef PT
#undef PTC

// counter of used channels (0 - timer OFF)
static uint8_t channel_counter[TIMERS_AMOUNT] = {0};

void pwm_setup(){
    // setup; start/stop only by user request
    for(int i = 1; i < TIMERS_AMOUNT; ++i){ // start from 1 as 0 forbidden
        volatile TIM_TypeDef *timer = timers[i];
        timer->CR1 = 0;
        timer->PSC = 7; // 6MHz for 23.4kHz PWM
        timer->ARR = 254; // 255 == 100%
        // PWM mode 1, preload enable
        timer->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE |
                       TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2PE;
        timer->CCMR2 = TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3PE |
                       TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4PE;
        timer->BDTR |= TIM_BDTR_MOE; // enable main output (need for some timers)
        timer->EGR |= TIM_EGR_UG; // force update generation
    }
    bzero(channel_counter, sizeof(channel_counter));
}

/**
 * @brief canPWM - check if pin have PWM ability
 * @param port - port (0/1 for GPIOA/GPIOB)
 * @param pin - pin (0..15)
 * @param t (o) - struct for pin's PWM timer
 * @return TRUE if can, FALSE if no
 */
int canPWM(uint8_t port, uint8_t pin, pwmtimer_t *t){
    if(port > 1 || pin > 15) return 0;
    if(t) *t = timer_map[port][pin];
    if(timer_map[port][pin].timidx == TIM_UNSUPPORTED) return FALSE;
    return TRUE;
}

/**
 * @brief startPWM - run PWM on given port/pin
 * @param port
 * @param pin
 * @return FALSE if unsupported
 */
int startPWM(uint8_t port, uint8_t pin){
    timidx_t idx = timer_map[port][pin].timidx;
    if(idx == TIM_UNSUPPORTED) return FALSE;
    volatile TIM_TypeDef *timer = timers[idx];
    uint8_t chidx = timer_map[port][pin].chidx;
    uint32_t chen = TIM_CCER_CC1E << (chidx<<2);
    volatile uint32_t *CCR = &timers[idx]->CCR1 + timer_map[port][pin].chidx;
    *CCR = 0; // set initial value to zero
    if(0 == (timer->CCER & chen)){
        if(0 == channel_counter[idx]++) timer->CR1 |= TIM_CR1_CEN; // start timer if need
        timer->CCER |= chen; // enable channel
    }
    return TRUE;
}

// stop given PWM channel and stop timer if there's no used channels
void stopPWM(uint8_t port, uint8_t pin){
    timidx_t idx = timer_map[port][pin].timidx;
    if(idx == TIM_UNSUPPORTED) return;
    volatile TIM_TypeDef *timer = timers[idx];
    uint8_t chidx = timer_map[port][pin].chidx;
    uint32_t chen = TIM_CCER_CC1E << (chidx<<2);
    if(timer->CCER & chen){
        if(0 == --channel_counter[idx]) timer->CR1 &= ~TIM_CR1_CEN; // stop timer
        timer->CCER &= ~chen;
    }
}

/**
 * @brief setPWM - set PWM value for given pin on given port
 * @param port
 * @param pin
 * @param val - 0..255
 * @return  FALSE if pin can't PWM
 */
int setPWM(uint8_t port, uint8_t pin, uint8_t val){
    timidx_t idx = timer_map[port][pin].timidx;
    if(idx == TIM_UNSUPPORTED) return FALSE;
    volatile uint32_t *CCR = &timers[idx]->CCR1 + timer_map[port][pin].chidx;
    *CCR = val;
    return TRUE;
}

/**
 * @brief getPWM - get PWM value for given pin on given port
 * @param port
 * @param pin
 * @return -1 if there's no PWM on that pin
 */
int16_t getPWM(uint8_t port, uint8_t pin){
    timidx_t idx = timer_map[port][pin].timidx;
    if(idx == TIM_UNSUPPORTED) return -1;
    volatile uint32_t *CCR = &timers[idx]->CCR1 + timer_map[port][pin].chidx;
    return (int16_t) *CCR;
}
