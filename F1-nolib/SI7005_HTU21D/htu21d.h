/*
 * This file is part of the HTU21D project.
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
#ifndef HTU21D_H__

#include <stm32f1.h>

typedef enum{
    HTU21D_BUSY,        // measurement in progress
    HTU21D_ERR,         // error in I2C
    HTU21D_RELAX,       // relaxed state
    HTU21D_TRDY,        // data ready - can get it
    HTU21D_HRDY
} HTU21D_status;

HTU21D_status HTU21D_get_status();

void HTU21D_setup();
int HTU21D_read_ID(uint8_t *devid);
int HTU21D_cmdT();
int HTU21D_cmdH();
void HTU21D_process();
int32_t HTU21D_getT();
uint32_t HTU21D_getH();

#define HTU21D_H__
#endif // HTU21D_H__
