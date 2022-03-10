/*
 * This file is part of the MAX7219 project.
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
#ifndef MAX_H__
#define MAX_H__

#include <stm32f1.h>

void MAX7219_test(int i);
void MAX7219_setup();
void MAX7219_setintens(uint8_t i);
void MAX7219_process();
int MAX7219_point(int x, int y, int s);
void MAX7219_refresh();
void MAX7219_rollscreen();

#endif // MAX_H__
