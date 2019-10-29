/*
 * This file is part of the chronometer project.
 * Copyright 2018 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#ifndef ADC_H
#define ADC_H
#include "stm32f1.h"

#define NUMBER_OF_ADC_CHANNELS (3)

// interval of trigger's shot (>min && <max), maybe negative
#define ADC_MIN_VAL     (1024)
#define ADC_MAX_VAL     (3072)
// 2*ADC_THRESHOLD = hysteresis width
#define ADC_THRESHOLD   (50)

extern uint16_t ADC_array[];
int32_t getMCUtemp();
uint32_t getVdd();
uint16_t getADCval(int nch);
uint8_t chkADCtrigger();
#endif // ADC_H
