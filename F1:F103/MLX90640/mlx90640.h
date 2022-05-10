/*
 * This file is part of the MLX90640 project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#ifndef MLX90640__
#define MLX90640__

#include <stm32f1.h>

// default I2C address
#define MLX_DEFAULT_ADDR    (0x33)

void mlx90640_process();
int read_reg(uint16_t reg, uint16_t *val);
int write_reg(uint16_t reg, uint16_t val);
int read_data(uint16_t reg, uint16_t *data, int N);
int read_data_dma(uint16_t reg, int N);

#endif // MLX90640__
