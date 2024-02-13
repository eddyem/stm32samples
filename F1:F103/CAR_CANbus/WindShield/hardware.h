/*
 * This file is part of the windshield project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32f1.h>

// frequency of TIM3 clocking
#define TIM3FREQ    (72000000)
// PWM frequency
#define PWMFREQ     (200000)
// max PWM value
#define PWMMAX      (100)

#define CONCAT(a,b)     a ## b
#define STR_HELPER(s)   #s
#define STR(s)          STR_HELPER(s)

#define FORMUSART(X)    CONCAT(USART, X)
#define USARTX          FORMUSART(USARTNUM)

// turn ON upper shoulder, x==0 - L_UP, x==1 - R_UP
#define UP_LEFT     0
#define UP_RIGHT    1
#define _set_up(x) do{GPIOA->BSRR = (x) ? (1<<3)|((1<<2)<<16) : (1<<2)|((1<<3)<<16); }while(0)
#define set_up(x) _set_up(x)
#define read_upL()  (pin_read(GPIOA, 1<<2))
#define read_upR()  (pin_read(GPIOA, 1<<3))
// turn off upper shoulders
#define up_off()  do{pin_clear(GPIOA, (1<<2)|(1<<3));}while(0)
#define start_pwm() do{ TIM3->CR1 = TIM_CR1_CEN; TIM3->EGR |= TIM_EGR_UG; }while(0)
#define stop_pwm()  do{ TIM3->CCR1 = 0; TIM3->CCR2 = 0; TIM3->CR1 = 0; }while(0)
#define PWM_LEFT    2
#define PWM_RIGHT   1
// set PWM value (0..100), x==1 - R_DOWN, x==2 - L_DOWN
#define _set_pwm(x, n)  do{ TIM3->CCR ## x = n; }while(0)
#define set_pwm(x, n) _set_pwm(x, n)
#define _get_pwm(x)    (TIM3->CCR ## x)
#define get_pwm(x) _get_pwm(x)

int set_dT(uint32_t d);

extern volatile uint32_t Tms;

void gpio_setup(void);
void iwdg_setup();

int motor_ctl(int32_t dir);
void motor_break();
void motor_process();


