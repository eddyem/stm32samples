/*
 * This file is part of the I2Ctiny project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

typedef enum{
	I2C_OK = 0,
	I2C_LINEBUSY,
	I2C_TMOUT,
	I2C_NOADDR,
	I2C_NACK,
	I2C_HWPROBLEM,
} i2c_status;

void i2c_setup();
void i2c_set_addr7(uint8_t addr);
i2c_status i2c_7bit_send_onebyte(uint8_t data, uint8_t stop);
i2c_status i2c_7bit_send(const uint8_t *data, int datalen);
i2c_status i2c_7bit_receive_onebyte(uint8_t *data, uint8_t stop);
i2c_status i2c_7bit_receive_twobytes(uint8_t *data);
i2c_status i2c_7bit_receive(uint8_t *data, uint16_t nbytes);

