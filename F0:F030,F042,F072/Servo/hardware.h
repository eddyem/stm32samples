/*
 * This file is part of the Chiller project.
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
#pragma once
#ifndef HARDWARE_H
#define HARDWARE_H
#include "stm32f0.h"

// minimal and maximal pulse length for SG90
#define SG90_MINPULSE   (700)
#define SG90_MAXPULSE   (2100)
#define SG90_MIDPULSE   ((SG90_MINPULSE+SG90_MAXPULSE)/2)
#define SG90_AMPL       (SG90_MAXPULSE-SG90_MINPULSE)
#define SG90DEFSTEP     (100)
#define SG90_STEP       (sg90step)

extern volatile uint32_t Tms;
extern uint32_t sg90step;

void hw_setup(void);
int32_t setPWM(int nch, uint32_t val, uint32_t speed);
int32_t getPWM(int nch);
uint8_t onposition(int nch);
void setTIM3T(uint32_t T);

#endif // HARDWARE_H
