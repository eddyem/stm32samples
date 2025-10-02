/*
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

#define I2C_ADDREND     (0x80)
#define I2C_BUFSIZE     (256)

typedef enum{
    I2C_SPEED_10K,
    I2C_SPEED_100K,
    I2C_SPEED_400K,
    I2C_SPEED_1M,
    I2C_SPEED_AMOUNT
} i2c_speed_t;

extern i2c_speed_t i2c_curspeed;
extern volatile uint8_t i2c_scanmode;

// timeout of I2C bus in ms
#define I2C_TIMEOUT     (5)

void i2c_setup(i2c_speed_t speed);

uint8_t *i2c_read(uint8_t addr, uint8_t nbytes);
uint8_t *i2c_read_regs(uint8_t addr, uint8_t reg, uint8_t nbytes);

uint8_t i2c_write(uint8_t addr, uint8_t *data, uint8_t nbytes);

void i2c_init_scan_mode();
int i2c_scan_next_addr(uint8_t *addr);
