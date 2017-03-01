/*
 *                                                                                                  geany_encoding=koi8-r
 * i2c.h
 *
 * Copyright 2017 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#define HTU21_ADDR          (0x40 << 1)
#define HTU21_READ_TEMP     (0xF3)
#define HTU21_READ_HUMID    (0xF5)
#define HTU21_READ_REG      (0xE7)
#define HTU21_WRITE_REG     (0xE6)
#define HTU21_SOFT_RESET    (0xFE)
// user reg fields
#define HTU21_REG_VBAT      (0x40)
#define HTU21_REG_D1        (0x80)
#define HTU21_REG_D0        (0x01)
#define HTU21_REG_HTR       (0x04)
#define HTU21_REG_ODIS      (0x02)

void i2c_setup();

int16_t convert_temperature(uint16_t in);
int16_t convert_humidity(uint16_t in);
uint8_t htu_write_reg(uint8_t data);
uint8_t htu_read_reg(uint8_t *data);
uint8_t htu_read_i2c(uint16_t *data);
uint8_t htu_write_i2c(uint8_t data);
