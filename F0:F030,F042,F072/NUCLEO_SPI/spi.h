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
#ifndef SPI_H__
#define SPI_H__

#include <stm32f0.h>

// 5 bytes == 40 bits
#define SPIBUFSZ    (2)

typedef enum{
    SPI_NOTREADY,
    SPI_READY,
    SPI_BUSY
} spiStatus;

extern spiStatus SPI_status[];
extern volatile uint8_t SPIoverfl[];

void spi_setup();
void SPI_prep_receive();
uint8_t SPI_getdata(uint8_t *buf, uint8_t *maxlen);

#endif // SPI_H__
