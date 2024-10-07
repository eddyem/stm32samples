/*
 * This file is part of the shutter project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "flash.h"

// PB12 - hall, PB10 - CCD (active low)
#define BTNPORT     GPIOB
#define HALLPIN     (1<<12)
#define CCDPIN      (1<<10)
#define CHKHALL()   ((HALLPIN == (BTNPORT->IDR & HALLPIN)) == the_conf.hallactive)
#define CHKCCD()    ((CCDPIN == (BTNPORT->IDR & CCDPIN)) == the_conf.ccdactive)

extern volatile uint32_t Tms;

void hw_setup();
uint32_t getShutterVoltage();
int set_pwm(uint32_t percent);
