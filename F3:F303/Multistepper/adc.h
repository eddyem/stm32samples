/*
 * This file is part of the multistepper project.
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
#define NUMBER_OF_ADC1_CHANNELS     (6)
// ext ADC channels (ADC_AINx)
#define NUMBER_OF_EXT_ADC_CHANNELS  (4)
// ADC2 channels
#define NUMBER_OF_ADC2_CHANNELS     (2)
// total number of channels - for array
#define NUMBER_OF_ADC_CHANNELS ((NUMBER_OF_ADC1_CHANNELS+NUMBER_OF_ADC2_CHANNELS))


// AIN0-3: ADC1_IN1..4; AIN4 - ADC2_IN1; AIN5 - ADC2_IN10
// channels of ADC in array
#define ADC_AIN0    (0)
#define ADC_AIN1    (1)
#define ADC_AIN2    (2)
#define ADC_AIN3    (3)
#define ADC_TS      (4)
#define ADC_VREF    (5)
#define ADC_VDRIVE  (6)
#define ADC_VFIVE   (7)
// starting index of ADC2
#define ADC2START   (9*NUMBER_OF_ADC1_CHANNELS)

void adc_setup();
int32_t getMCUtemp();
int32_t getVdd();
uint16_t getADCval(int nch);
int32_t getADCvoltage(int nch);
