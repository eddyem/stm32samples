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

// CSB=1, address 1110110
#define TSYS01_ADDR0            (0x76 << 1)
// CSB=0, address 1110111
#define TSYS01_ADDR1            (0x77 << 1)
// registers: reset, read ADC value, start converstion, sart of PROM
#define TSYS01_RESET            (0x1E)
#define TSYS01_ADC_READ         (0x00)
#define TSYS01_START_CONV       (0x48)
#define TSYS01_PROM_ADDR0       (0xA0)
// conversion time = 10ms
#define CONV_TIME               (10)

void i2c_setup();

uint8_t read_i2c(uint8_t addr, uint32_t *data, uint8_t nbytes);
uint8_t write_i2c(uint8_t addr, uint8_t data);
