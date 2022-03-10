/*
 * This file is part of the TM1637 project.
 * Copyright 2019 Edward Emelianov <eddy@sao.ru>.
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

#ifndef I2C_H__
#define I2C_H__

#include "stm32f0.h"

// timeout in ms
#define I2C_TIMEOUT             (15)

uint8_t read_i2c(uint8_t command, uint8_t *data);
uint8_t write_i2c(const uint8_t *commands, uint8_t nbytes);

#endif // I2C_H__
