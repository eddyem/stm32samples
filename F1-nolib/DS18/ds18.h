/*
 * This file is part of the DS18 project.
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


// state of device
typedef enum{
    DS18_SLEEP,
    DS18_RESETTING,
    DS18_DETECTING,
    DS18_DETDONE,
    DS18_RDYTOSEND,
    DS18_READING,
    DS18_GOTANSWER,
    DS18_ERROR
} DS18_state;

void DS18_pinsetup();
int DS18_start();
int DS18_readID();
DS18_state DS18_getstate();
void DS18_process(uint32_t Tms);
int DS18_readScratchpad();
void DS18_poll();

#endif // DHT_H__

