/*
 * This file is part of the windshield project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32f1.h>

#define NUMBER_OF_ADC_CHANNELS (3)

// Vsen voltage channel
#define CHVSEN  (0)
// channels for Tsens and Vdd
#define CHTSENS (1)
#define CHVDD   (2)

/**
 * @brief ADC_array - array for ADC channels with median filtering:
 * 0 - Isensor voltage
 * 1 - internal Tsens
 * 2 - Vdd
 */
extern uint16_t ADC_array[];

void adc_setup();
int32_t getMCUtemp();
uint32_t getVdd();
uint16_t getADCval(int nch);
uint32_t getADCvoltage(int nch);

