/*
 * This file is part of the nitrogen project.
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
#include <stm32f3.h>

// ADC1 channels
#define NUMBER_OF_ADC1_CHANNELS     (11)
// ADC2 channels
#define NUMBER_OF_ADC2_CHANNELS     (1)
// total number of channels - for array
#define NUMBER_OF_ADC_CHANNELS ((NUMBER_OF_ADC1_CHANNELS+NUMBER_OF_ADC2_CHANNELS))


// ADC1 in1..10 - inner and outern Tsens; ADC2 in1 - ext ADC
// channels of ADC in array
#define ADC_AIN0    (0)
#define ADC_AIN1    (1)
#define ADC_AIN2    (2)
#define ADC_AIN3    (3)
#define ADC_AIN4    (4)
#define ADC_AIN5    (5)
#define ADC_AIN6    (6)
#define ADC_AIN7    (7)
#define ADC_AIN8    (8)
#define ADC_AIN9    (9)
#define MERG(a,b)  a ## b
#define ADC_CHAN(x) MERG(ADC_AIN, x)
#define ADC_TSENS   (10)
#define ADC_EXT     (11)
// starting index of ADC2
#define ADC2START   (9*NUMBER_OF_ADC1_CHANNELS)

void adc_setup();
float getMCUtemp();
uint16_t getADCval(int nch);
float getADCvoltage(int nch);
