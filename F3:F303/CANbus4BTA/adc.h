/*
 * This file is part of the canbus4bta project.
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
#define NUMBER_OF_ADC_CHANNELS     (5)
// channels of ADC in array
#define ADC_AIN0    (0)
#define ADC_AIN1    (1)
#define ADC_AIN2    (2)
#define ADC_AIN3    (3)
#define MERG(a,b)  a ## b
#define ADC_CHAN(x) MERG(ADC_AIN, x)
#define ADC_TSENS   (4)

void adc_setup();
float getMCUtemp();
uint16_t getADCval(int nch);
float getADCvoltage(uint16_t ADCval);
