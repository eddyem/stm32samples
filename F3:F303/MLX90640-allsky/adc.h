/*
 * This file is part of the ir-allsky project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include <stm32f3.h>

// 4 sensors on 1..4, TS (16) and Vdd (18)
#define NUMBER_OF_ADC1_CHANNELS (6)
#define NUMBER_OF_ADC2_CHANNELS (1)
// total number of channels - for array
#define NUMBER_OF_ADC_CHANNELS ((NUMBER_OF_ADC1_CHANNELS+NUMBER_OF_ADC2_CHANNELS))

// channels of ADC in array
#define ADC_AIN1        (0)
#define ADC_AIN2        (1)
#define ADC_AIN3        (2)
#define ADC_AIN4        (3)
#define ADC_NTCIN(x)    ((x)-1)
#define ADC_TS          (4)
#define ADC_VREF        (5)
#define ADC_DACIN       (6)
// starting index of ADC2
#define ADC2START   (9*NUMBER_OF_ADC1_CHANNELS)

void adc_setup();
float getMCUtemp();
float getVdd();
uint16_t getADCval(uint8_t nch);
float getADCvoltage(uint8_t nch);
float getNTCtemp(uint8_t nch);
