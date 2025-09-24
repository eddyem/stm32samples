/*
 * This file is part of the mlx90640 project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "mlx90640.h"

// amount of sensors processing
#define N_SESORS            (5)

// maximal errors number to stop processing
#define MLX_MAX_ERRORS      (11)
// if there's no new data by this time - reset bus
#define MLX_I2CERR_TMOUT    (5000)

typedef enum{
    MLX_NOTINIT,        // just start - need to get parameters
    MLX_WAITPARAMS,     // wait for parameters DMA reading
    MLX_WAITSUBPAGE,    // wait for subpage changing
    MLX_READSUBPAGE,    // wait ending of subpage DMA reading
    MLX_RELAX           // do nothing - pause
} mlx_state_t;

int mlx_setaddr(int n, uint8_t addr);
mlx_state_t mlx_state();
int mlx_nactive();
uint8_t *mlx_activeids();
void mlx_pause();
void mlx_stop();
void mlx_continue();
void mlx_process();
MLX90640_params *mlx_getparams(int sensno);
fp_t *mlx_getimage(int sensno);
int mlx_sethwaddr(uint8_t MLX_address, uint8_t addr);
uint32_t mlx_lastimT(int sensno);
