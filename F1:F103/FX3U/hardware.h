/*
 * This file is part of the fx3u project.
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


#define CONCAT(a,b)     a ## b
#define STR_HELPER(s)   #s
#define STR(s)          STR_HELPER(s)

/* this stuff may be usefull later
#define FORMUSART(X)    CONCAT(USART, X)
#define USARTX          FORMUSART(USARTNUM)
*/

// max number of in/out pins
#define INMAX       (15)
#define OUTMAX      (11)

// onboard LED - PD10
#define LEDPORT GPIOD
#define LEDPIN  (1<<10)

extern volatile uint32_t Tms;

void gpio_setup(void);
void iwdg_setup();
int set_relay(uint8_t Nch, uint32_t val);
int get_relay(uint8_t Nch);
int get_esw(uint8_t Nch);
void proc_esw();
uint32_t get_ab_esw();

uint8_t LED(int onoff);
uint32_t inchannels();
uint32_t outchannels();
int set_relay(uint8_t Nch, uint32_t val);
int get_relay(uint8_t Nch);
int get_esw(uint8_t Nch);
