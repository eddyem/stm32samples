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

// timeout for reading operations, ms
#define MLX_TIMEOUT         1000
// counter of errors, when > max -> M_ERROR
#define MLX_MAXERR_COUNT    10
// wait after power off, ms
#define MLX_POWOFF_WAIT     500
// wait after power on, ms
#define MLX_POWON_WAIT      2000

// amount of pixels
#define MLX_PIXNO           (24*32)
// pixels + service data
#define MLX_PIXARRSZ        (MLX_PIXNO + 64)

typedef struct{
    int16_t kVdd;
    int16_t vdd25;
    float KvPTAT;
    float KtPTAT;
    int16_t vPTAT25;
    float alphaPTAT;
    int16_t gainEE;
    float tgc;
    float cpKv;     // K_V_CP
    float cpKta;    // K_Ta_CP
    float KsTa;
    float CT[3]; // range borders (0, 160, 320 degrC?)
    float ksTo[4]; // K_S_To for each range
    float alphacorr[4]; // Alpha_corr for each range
    float alpha[MLX_PIXNO]; // full - with alpha_scale
    int16_t offset[MLX_PIXNO];
    float kta[MLX_PIXNO]; // full K_ta - with scale1&2
    float kv[4];  // full - with scale; 0 - odd row, odd col; 1 - odd row even col; 2 - even row, odd col; 3 - even row, even col
    float cpAlpha[2];   // alpha_CP_subpage 0 and 1
    int16_t cpOffset[2];
} MLX90640_params;

extern MLX90640_params params;

typedef enum{
    M_ERROR,            // error: need to reboot sensor
    M_RELAX,            // base state
    M_FIRSTSTART,       // first start after power on
    M_READCONF,         // read configuration data
    M_STARTIMA,         // start image aquiring
    M_PROCESS,          // process subpage - wait for image ready
    M_READOUT,          // wait while subpage data be read
    M_POWERON,          // wait for 100ms after power is on before -> firststart
    M_POWEROFF1,        // turn off power
    M_POWEROFF,         // wait for 500ms without power
    //
    M_STATES_AMOUNT     // amount of states
} mlx90640_state;

extern mlx90640_state mlx_state;
extern float mlx_image[MLX_PIXNO];

// default I2C address
#define MLX_DEFAULT_ADDR    (0x33)
// max datalength by one read (in 16-bit values)
#define MLX_DMA_MAXLEN      (832)

int read_reg(uint16_t reg, uint16_t *val);
int write_reg(uint16_t reg, uint16_t val);
int read_data(uint16_t reg, uint16_t *data, int N);
int read_data_dma(uint16_t reg, int N);
void mlx90640_process();
int mlx90640_take_image(uint8_t simple);
void mlx90640_restart();

#endif // MLX90640__
