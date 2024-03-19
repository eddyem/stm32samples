/*
 * This file is part of the canbus4bta project.
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
#include <stm32f3.h>

#define AMOUNT_OF_SPI   (2)

typedef enum{
    SPI_NOTREADY,
    SPI_READY,
    SPI_BUSY
} spiStatus;

extern spiStatus spi_status[AMOUNT_OF_SPI+1];

void spi_onoff(uint8_t idx, uint8_t on);
void spi_deinit(uint8_t idx);
void spi_setup(uint8_t idx);
int spi_waitbsy(uint8_t idx);
int spi_writeread(uint8_t idx, uint8_t *data, uint32_t n);
int spi_read(uint8_t idx, uint8_t *data, uint32_t n);
//int spi_write_dma(const uint8_t *data, uint8_t *rxbuf, uint32_t n);
//int spi_read(uint8_t idx, uint8_t *data, uint32_t n);
//uint8_t *spi_read_dma(uint32_t *n);
