/*
 * This file is part of the DHT22 project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#ifndef DHT_H__
#define DHT_H__

#include <stm32f1.h>

// define it to work with DHT11
//#define DHT11

// reset pulse length in ms
#define DHT_RESETPULSE_LEN      (20)
// max measurement length
#define DHT_MEASUR_LEN          (50)

// state of device
typedef enum{
    DHT_SLEEP,
    DHT_RESETTING,
    DHT_MEASURING,
    DHT_GOTRESULT,
    DHT_ERROR
} DHT_state;

void DHT_pinsetup();
int DHT_start(uint32_t Tms);
DHT_state DHT_getstate();
void DHT_process(uint32_t Tms);
void DHT_getdata(uint16_t *Hum, int16_t *T);

#endif // DHT_H__

