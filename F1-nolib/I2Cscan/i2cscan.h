/*
 * This file is part of the I2Cscan project.
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
#ifndef I2CSCAN_H__
#define I2CSCAN_H__

#include <stm32f1.h>

#define I2C_ADDREND  (0x80)

extern uint8_t scanmode;

uint8_t getI2Caddr();

void init_scan_mode();
int scan_next_addr(uint8_t *addr);

int read_reg(uint8_t reg, uint8_t *val, uint8_t N);
int write_reg(uint8_t reg, uint8_t val);


#endif // I2CSCAN_H__
