/**
 * Ciastkolog.pl (https://github.com/ciastkolog)
 *
*/
/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 sheinz (https://github.com/sheinz)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/*
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#ifndef BMP280_H__
#define BMP280_H__

#include <stm32f3.h>

#define BMP280_CHIP_ID  0x58
#define BME280_CHIP_ID  0x60

typedef enum{ // K for filtering: next = [prev*(k-1) + data_ADC]/k
    BMP280_FILTER_OFF = 0, // k=1, no filtering
    BMP280_FILTER_2   = 1, // k=2, 2 samples to reach >75% of data_ADC
    BMP280_FILTER_4   = 2, // k=4, 5 samples
    BMP280_FILTER_8   = 3, // k=8, 11 samples
    BMP280_FILTER_16  = 4, // k=16, 22 samples
    BMP280_FILTERMAX
} BMP280_Filter;

typedef enum{ // Number of oversampling
    BMP280_NOMEASUR = 0,
    BMP280_OVERS1   = 1,
    BMP280_OVERS2   = 2,
    BMP280_OVERS4   = 3,
    BMP280_OVERS8   = 4,
    BMP280_OVERS16  = 5,
    BMP280_OVERSMAX
} BMP280_Oversampling;

typedef enum{
    BMP280_NOTINIT,     // wasn't inited
    BMP280_BUSY,        // measurement in progress
    BMP280_ERR,         // error in I2C
    BMP280_RELAX,       // relaxed state
    BMP280_RDY,         // data ready - can get it
} BMP280_status;


void BMP280_setup(uint8_t address, uint8_t isI2C);
int BMP280_init();
void BMP280_setfilter(BMP280_Filter f);
BMP280_Filter BMP280_getfilter();
void BMP280_setOSt(BMP280_Oversampling os);
void BMP280_setOSp(BMP280_Oversampling os);
void BMP280_setOSh(BMP280_Oversampling os);
int BMP280_read_ID(uint8_t *devid);
BMP280_status BMP280_get_status();
int BMP280_start();
void BMP280_process();
int BMP280_getdata(float *T, float *P, float *H);
float Tdew(float T, float H);

#endif // BMP280_H__
