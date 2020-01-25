/*
 * This file is part of the chronometer project.
 * Copyright 2019 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#ifndef TIME_H__
#define TIME_H__

#include <stm32f1.h>

// default value for systick_config
#define SYSTICK_DEFCONF         (72000)
// defaul for systick->load
#define SYSTICK_DEFLOAD         (SYSTICK_DEFCONF - 1)
#define TIMEZONE_GMT_PLUS       (3)

#define DIDNT_TRIGGERED (2000)

// debounce delay: .4s
#define TRIGGER_DELAY     (400)

#define TMNOTINI  {25,61,61}

// current milliseconds
#define get_millis()  (Timer)

typedef struct{
    uint8_t H;
    uint8_t M;
    uint8_t S;
} curtime;

extern volatile uint32_t Tms;
extern volatile uint32_t Timer;
extern curtime current_time;

extern curtime trigger_time[];
extern uint32_t trigger_ms[];

extern volatile int need_sync;

char *get_time(curtime *T, uint32_t m);
void set_time(const char *buf);
void time_increment();
void systick_correction();

#endif // TIME_H__
