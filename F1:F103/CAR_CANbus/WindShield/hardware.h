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
#define PWMFREQ     (2000)
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
#define stop_pwm()  do{ TIM3->CCR1 = 0; TIM3->CCR2 = 0; TIM3->CR1 = TIM_CR1_OPM | TIM_CR1_CEN; }while(0)
#define PWM_LEFT    2
#define PWM_RIGHT   1
// set PWM value (0..100), x==1 - R_DOWN, x==2 - L_DOWN
#define _set_pwm(x, n)  do{ TIM3->CCR ## x = n; }while(0)
#define set_pwm(x, n) _set_pwm(x, n)
#define _get_pwm(x)    (TIM3->CCR ## x)
#define get_pwm(x) _get_pwm(x)

// buttons, dir, hall:
#define HALL_PORT       GPIOA
#define HALL_U_PIN      (1<<12)
#define HALL_D_PIN      (1<<11)
#define BUTTON_PORT     GPIOB
#define BUTTON_U_PIN    (1<<14)
#define BUTTON_D_PIN    (1<<15)
#define DIR_PORT        GPIOB
#define DIR_U_PIN       (1<12)
#define DIR_D_PIN       (1<13)

// define BUTTONS_NEGATIVE if button pressed when ==1
#ifdef BUTTONS_NEGATIVE
#define PRESSED(port, pin)      ((port->IDR & pin) == pin)
#else
#define PRESSED(port, pin)      ((port->IDR & pin) == 0)
#endif

// moving when > NONE, stopped when < NONE
typedef enum{
    MOTDIR_STOP = -2,   // stop with deceleration
    MOTDIR_BREAK = -1,  // emergency stop
    MOTDIR_NONE = 0,    // do nothing
    MOTDIR_UP = 1,      // move up
    MOTDIR_DOWN = 2,    // move down
} motdir_t;

int set_dT(uint32_t d);

extern volatile uint32_t Tms;

void gpio_setup(void);
void iwdg_setup();

int motor_ctl(int32_t dir);
void motor_break();
void motor_process();


