/*
 * adc.h
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
#ifndef __SHARP_H__
#define __SHARP_H__

#include <stdint.h>

extern uint16_t ADC_value[];
extern uint16_t ADC_trig_val[];

typedef enum{
	ADWD_LOW,  // watchdog at low-level
	ADWD_MID,  // normal state
	ADWD_HI    // watchdog at high-level
} adwd_stat;

extern adwd_stat adc_status[];


#define ADC_CHANNEL_NUMBER (2)

// something near
#define ADC_WDG_HIGH   ((uint16_t)1500)
// nothing in front of sensor
#define ADC_WDG_LOW    ((uint16_t)700)
// threshold above levels
#define ADC_WDG_THRES  ((uint16_t)200)

void init_adc_sensor();
void poll_ADC();

#endif // __SHARP_H__
