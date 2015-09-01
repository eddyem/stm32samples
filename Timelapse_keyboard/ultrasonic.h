/*
 * ultrasonic.h
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
#ifndef __ULTRASONIC_H__
#define __ULTRASONIC_H__

#include <stdint.h>

void tim2_init();
int ultrasonic_get(uint32_t *L);
void poll_ultrasonic();
extern uint32_t last_us_val;

// Sensor mode
typedef enum{
	 US_MODE_OFF         // sensor is off
	,US_MODE_DONE        // measurement done, ready for next
	,US_MODE_TRIG        // sensor triggered
	,US_MODE_WAIT        // wait for pulse
	,US_MODE_MEASUREMENT // measurement in process
	,US_MODE_READY       // measurements done
} usmode;

// trigger time: after trigger event ends, timer will be configured for capture - 20us
#define TRIG_T     (200)
// trigger length - 10us
#define TRIG_L     (10)
// max length of measurement (to detect signal absence)
#define MAX_MSRMNT_LEN  (65535)
// max difference of measured len for pass detection
#define MAX_LEN_DIFF   (300)
// minimal length of signal in ms
#define ULTRASONIC_TIMEOUT  (10)
#endif // __ULTRASONIC_H__
