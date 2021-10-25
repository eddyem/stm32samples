/*
 * This file is part of the 3steppers project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#ifndef STEPPERS_H__
#define STEPPERS_H__

#include <stm32f0.h>
#include "commonproto.h"

// direction
extern int8_t motdir[];

void addmicrostep(int i);
void encoders_UPD(int i);

void init_steppers();
errcodes getpos(int i, int32_t *position);
errcodes setpos(int i, int32_t newpos);
void stopmotor(int i);

#endif // STEPPERS_H__
