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
#pragma once
#ifndef HARDWARE_H
#define HARDWARE_H
#include "stm32f0.h"

// measure flow sensor data each 1 second
#define FLOW_RATE_MS        (999)
// previous as string constant
#define FLOWRATESTR         "1"

// each TMEASURE_MS ms calculate temperatures & check them
#define TMEASURE_MS         (1000)
// each TCHECK_MS ms check cooler state and regulate temperature
#define TCHECK_MS           (10000)

/*
                temperature limits and tolerances
 */
// tolerance: +-1.5degrC
#define TEMP_TOLERANCE      (15)
// dT tolerance: +-0.5degrC
#define DT_TOLERANCE        (5)
// maximal heater temperature - 80degrC; normal - <60
#define MAX_HEATER_T        (800)
#define NORMAL_HEATER_T     (600)
// maximal output temperature - 45degrC; minimal - 10
#define MAX_OUTPUT_T        (450)
#define MIN_OUTPUT_T        (100)
// temperature working values: from 15 to 30degrC
#define OUTPUT_T_H          (300)
#define OUTPUT_T_L          (150)

/*
                other limits & tolerances
*/
// minimal flow rate - 0.2l per minute
#define MIN_FLOW_RATE       (20)
// normal flow rate
#define NORMAL_FLOW_RATE    (30)
// minimal PWM values when motors should work
#define MIN_PUMP_PWM        (90)
#define MIN_COOLER_PWM      (90)

// PWM setters and getters
#define SET_COOLER_PWM(N)   do{TIM14->CCR1 = (uint32_t)N;}while(0)
#define GET_COOLER_PWM()    (uint16_t)(TIM14->CCR1)
#define SET_HEATER_PWM(N)   do{TIM16->CCR1 = (uint32_t)N;}while(0)
#define GET_HEATER_PWM()    (uint16_t)(TIM16->CCR1)
#define SET_PUMP_PWM(N)     do{TIM17->CCR1 = (uint32_t)N;}while(0)
#define GET_PUMP_PWM()      (uint16_t)(TIM17->CCR1)

// ext. alarm states
#define ALARM_ON()          pin_set(GPIOF, 2)
#define ALARM_OFF()         pin_clear(GPIOF, 2)
#define ALARM_STATE()       pin_read(GPIOF, 2)

extern volatile uint16_t flow_rate, flow_cntr;
extern volatile uint32_t Tms;

void hw_setup(void);

#endif // HARDWARE_H
