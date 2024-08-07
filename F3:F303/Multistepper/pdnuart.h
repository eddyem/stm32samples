/*
 * This file is part of the multistepper project.
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

#include <stdint.h>

void pdnuart_setup();
int pdnuart_writereg(uint8_t reg, uint32_t data);
int pdnuart_readreg(uint8_t reg, uint32_t *data);
int pdnuart_setmotno(uint8_t no);
uint8_t pdnuart_getmotno();
int pdnuart_setcurrent(uint8_t no, uint8_t val);
int pdnuart_microsteps(uint8_t no, uint32_t val);
int pdnuart_init(uint8_t no);
