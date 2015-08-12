/*
 * steppers.h
 *
 * Copyright 2015 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#pragma once
#ifndef __STEPPERS_H__
#define __STEPPERS_H__

#include "main.h"
// minimal period - 100us (10000ticks per second)
#define STEPPER_MIN_PERIOD      (99)
// default period
#define STEPPER_DEFAULT_PERIOD  (1299)
// high (off) pulse length: 70us (minimum value according documentation is 0.5us)
#define STEPPER_PULSE_LEN       (69)
// TIM2 have 16bit ARR register, so max P with fixed prescaler is 65536
#define STEPPER_TIM_MAX_ARRVAL  (65536)
// max period allowed: 6553600us = 6.5s
#define STEPPER_MAX_PERIOD      (6553601)
// default prescaler for 1MHz
#define STEPPER_TIM_DEFAULT_PRESCALER  (71)
// prescaler for 100kHz
#define STEPPER_TIM_HUNDR_PRESCALER    (7199)
void steppers_init();

uint8_t set_stepper_speed(int32_t Period);
uint8_t move_stepper(int32_t Nsteps);
void stop_stepper();
extern volatile int32_t Glob_steps;
#define stepper_get_steps()  (Glob_steps)

extern int32_t stepper_period;
int32_t stepper_get_period();
#endif // __STEPPERS_H__
