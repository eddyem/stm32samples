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

#pragma once

#include <stdint.h>

// used timers
typedef enum{
    TIM_UNSUPPORTED = 0,
    TIM1_IDX,
    TIM2_IDX,
    TIM3_IDX,
    TIM14_IDX,
    TIM16_IDX,
    TIMERS_AMOUNT
}timidx_t;

// Timers for PWM
typedef struct{
    timidx_t timidx : 3;        // timer index from array of timers used
    uint8_t chidx : 2;          // channel index (0..3)
    uint8_t collision : 1;      // have collision with other channel (1)
    uint8_t collport : 1;       // collision port index (0 - GPIOA, 1 - GPIOB)
    uint8_t collpin : 4;        // collision pin index (0..15)
} pwmtimer_t;


void pwm_setup();
int canPWM(uint8_t port, uint8_t pin, pwmtimer_t *t);
int startPWM(uint8_t port, uint8_t pin);
void stopPWM(uint8_t port, uint8_t pin);
int setPWM(uint8_t port, uint8_t pin, uint8_t val);
int16_t getPWM(uint8_t port, uint8_t pin);
