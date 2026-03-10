/*
 * This file is part of the usbcangpio project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include <stdint.h>

// 10 ADC ins + VDD + MCUt
#define NUM_EXT_ADC_CH   10
#define MAX_ADC_CHANNELS    (NUM_EXT_ADC_CH+2)

int32_t getMCUtemp();
uint32_t getVdd();
uint16_t getADCval(int nch);
void adc_setup();
uint16_t *getUval();
