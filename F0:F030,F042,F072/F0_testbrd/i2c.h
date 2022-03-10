/*
 * This file is part of the F0testbrd project.
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
#ifndef I2C_H__
#define I2C_H__

#include "stm32f0.h"

#define I2C_ADDREND  (0x80)

typedef enum{
    VERYLOW_SPEED,
    LOW_SPEED,
    HIGH_SPEED,
    CURRENT_SPEED
} I2C_SPEED;

extern I2C_SPEED curI2Cspeed;
extern volatile uint8_t I2C_scan_mode;

// timeout of I2C bus in ms
#define I2C_TIMEOUT             (100)

void i2c_setup(I2C_SPEED speed);
uint8_t read_i2c(uint8_t addr, uint8_t *data, uint8_t nbytes);
uint8_t read_i2c_reg(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t nbytes);
uint8_t write_i2c(uint8_t addr, uint8_t *data, uint8_t nbytes);

void i2c_init_scan_mode();
int i2c_scan_next_addr(uint8_t *addr);

#endif // I2C_H__
