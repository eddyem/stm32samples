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

// channels: 0 - IR, 1 - laser's photoresistor, 6 - 12V
#define ADC_CHANNEL_NUMBER (3)
// 10.8V - power alarm (resistor divider: 10kOhm : 3.0kOhm, U/100=7/20*ADC_value)
#define POWER_ALRM_LEVEL   (3086)
// 11.5V - power OK
#define GOOD_POWER_LEVEL   (3286)

void init_adc_sensor();
void poll_ADC();

#endif // __SHARP_H__
