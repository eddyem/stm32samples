/*
 * This file is part of the BMP180 project.
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
#ifndef BMP180_H__
#define BMP180_H__

#include <stm32f1.h>

#define BMP180_CHIP_ID  0x55

typedef enum{
    BMP180_NOTINIT,     // wasn't inited
    BMP180_BUSYT,       // T measurement in progress
    BMP180_BUSYP,       // P measurement in progress
    BMP180_ERR,         // error in I2C
    BMP180_RELAX,       // relaxed state
    BMP180_RDY,         // data ready - can get it
} BMP180_status;

typedef enum{
    BMP180_OVERS_1 = 0, // oversampling is off
    BMP180_OVERS_2 = 1,
    BMP180_OVERS_4 = 2,
    BMP180_OVERS_8 = 3,
    BMP180_OVERSMAX = 4
} BMP180_oversampling;


int BMP180_reset();
int BMP180_init();
void BMP180_read_ID(uint8_t *devid);
void BMP180_setOS(BMP180_oversampling os);
BMP180_status BMP180_get_status();
int BMP180_start(/*uint32_t curr_milliseconds*/);
void BMP180_process(/*uint32_t curr_milliseconds*/);
void BMP180_getdata(int32_t *T, uint32_t *P);

#endif // BMP180_H__
