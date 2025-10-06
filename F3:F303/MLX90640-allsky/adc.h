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

#define NUMBER_OF_ADC1_CHANNELS (4)
#define NUMBER_OF_ADC2_CHANNELS (1)
// total number of channels - for array
#define NUMBER_OF_ADC_CHANNELS ((NUMBER_OF_ADC1_CHANNELS+NUMBER_OF_ADC2_CHANNELS))

// channels of ADC in array
#define ADC_AIN0    (0)
#define ADC_AIN1    (1)
#define ADC_TS      (2)
#define ADC_VREF    (3)
#define ADC_AIN5    (4)
// starting index of ADC2
#define ADC2START   (9*NUMBER_OF_ADC1_CHANNELS)

void adc_setup();
float getMCUtemp();
float getVdd();
uint16_t getADCval(int nch);
float getADCvoltage(int nch);
