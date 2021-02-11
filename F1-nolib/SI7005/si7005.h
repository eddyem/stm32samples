/*
 * This file is part of the si7005 project.
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
#ifndef SI7005_H__
#define SI7005_H__

#include <stm32f1.h>

typedef enum{
    SI7005_BUSY,        // measurement in progress
    SI7005_ERR,         // error in I2C
    SI7005_RELAX,       // relaxed state
    SI7005_TRDY,        // data ready - can get it
    SI7005_HRDY
} si7005_status;

si7005_status si7005_get_status();

void si7005_setup();
int si7005_read_ID(uint8_t *devid);
int si7005_cmdT();
int si7005_cmdH();
void si7005_process();
int32_t si7005_getT();
uint32_t si7005_getH();

#endif // SI7005_H__
