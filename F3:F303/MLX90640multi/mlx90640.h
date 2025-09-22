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

#include "mlx90640_regs.h"

#include <stdint.h>

// floating type & sqrt operator
typedef float fp_t;
#define SQRT(x)     sqrtf((x))

// amount of pixels
#define MLX_W               (32)
#define MLX_H               (24)
#define MLX_PIXNO           (MLX_W*MLX_H)
// pixels + service data
#define MLX_PIXARRSZ        (MLX_PIXNO + 64)

typedef struct{
    int16_t kVdd;
    int16_t vdd25;
    fp_t KvPTAT;
    fp_t KtPTAT;
    int16_t vPTAT25;
    fp_t alphaPTAT;
    int16_t gainEE;
    fp_t tgc;
    fp_t cpKv;     // K_V_CP
    fp_t cpKta;    // K_Ta_CP
    fp_t KsTa;
    fp_t CT[3]; // range borders (0, 160, 320 degrC?)
    fp_t KsTo[4]; // K_S_To for each range * 273.15
    fp_t alphacorr[4]; // Alpha_corr for each range
    fp_t alpha[MLX_PIXNO]; // full - with alpha_scale
    fp_t offset[MLX_PIXNO];
    fp_t kta[MLX_PIXNO]; // full K_ta - with scale1&2
    fp_t kv[4];  // full - with scale; 0 - odd row, odd col; 1 - odd row even col; 2 - even row, odd col; 3 - even row, even col
    fp_t cpAlpha[2];   // alpha_CP_subpage 0 and 1
    uint8_t resolEE; // resolution_EE
    int16_t cpOffset[2];
    uint8_t outliers[MLX_PIXNO]; // outliers - bad pixels (if == 1)
} MLX90640_params;

int ch_resolution(uint8_t newresol);
int get_parameters(const uint16_t dataarray[MLX_DMA_MAXLEN], MLX90640_params *params);
fp_t *process_image(const MLX90640_params *params, const int16_t subpage1[REG_IMAGEDATA_LEN]);
void dumpIma(const fp_t im[MLX_PIXNO]);
void drawIma(const fp_t im[MLX_PIXNO]);
