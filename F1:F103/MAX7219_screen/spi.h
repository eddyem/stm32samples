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
#ifndef MAX7219_H__
#define MAX7219_H__

#include "stm32f1.h"

typedef enum{
    SPI_NOTREADY,
    SPI_READY,
    SPI_BUSY
} spiStatus;

extern spiStatus SPI_status;

void SPI_setup();
uint8_t SPI_transmit(const uint16_t *buf, uint8_t len);
void SPI_blocktransmit(const uint16_t data);
#endif // MAX7219_H__
