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

extern uint16_t ADC_array[];
int32_t getMCUtemp();
uint32_t getVdd();
uint16_t getADCval(int nch);
void chkADCtrigger();
#endif // ADC_H
