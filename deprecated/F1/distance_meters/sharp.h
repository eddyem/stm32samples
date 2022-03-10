/*
 * sharp.h
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

extern int AWD_flag;
extern uint16_t AWD_value;

// > 2.5V  - something nearest 1m
#define ADC_WDG_HIGH   ((uint16_t)3000)
// < 0.6V - nothing in front of sensor
#define ADC_WDG_LOW    ((uint16_t)750)

void init_sharp_sensor();

#endif // __SHARP_H__
