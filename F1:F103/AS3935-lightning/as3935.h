/*
 * This file is part of the as3935 project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

enum AS3935_REGISTERS{
    AFE_GAIN = 0x00,
    THRESHOLD,
    LIGHTNING_REG,
    INT_MASK_ANT,
    S_LIG_L,
    S_LIG_M,
    S_LIG_MM,
    DISTANCE,
    TUN_DISP,
    CALIB_TRCO = 0x3A,
    CALIB_SRCO = 0x3B,
    PRESET_DEFAULT = 0x3C,
    CALIB_RCO = 0x3D
};

// REGISTERS

typedef union{
    struct{
    uint8_t PWD : 1;
    uint8_t AFE_GB : 5;
    uint8_t RESERVED : 2;
    }; uint8_t u8;
} t_afe_gain;

typedef union{
    struct{
    uint8_t WDTH : 4;
    uint8_t NF_LEV : 3;
    uint8_t RESERVED : 1;
    }; uint8_t u8;
} t_threshold;

typedef union{
    struct{
    uint8_t SREJ : 4;
// minimal number of lightnings
#define NUM_LIG_1   (0)
#define NUM_LIG_5   (1)
#define NUM_LIG_9   (2)
#define NUM_LIG_16  (3)
    uint8_t MIN_NUM_LIG : 2;
    uint8_t CL_STAT : 1;
    uint8_t RESERVED : 1;
    }; uint8_t u8;
} t_lightning_reg;

typedef union{
    struct{
// interrupt flags
// noice level too high
#define INT_NH  (1)
// disturber detected
#define INT_D   (4)
// lightning interrupt
#define INT_L   (8)
    uint8_t INT : 4;
    uint8_t RESERVED : 1;
    uint8_t MASK_DIST : 1;
    uint8_t LCO_FDIV : 2;
    }; uint8_t u8;
} t_int_mask_ant;

typedef union{
    struct{
    uint8_t S_LIG_MM : 5;
    uint8_t RESERVED : 3;
    }; uint8_t u8;
} t_s_lig_mm;

typedef union{
    struct{
    uint8_t DISTANCE : 6;
    uint8_t RESERVED : 2;
    }; uint8_t u8;
} t_distance;

typedef union{
    struct{
    uint8_t TUN_CAP : 4;
    uint8_t RESERVED : 1;
    uint8_t DISP_TRCO : 1;
    uint8_t DISP_SRCO : 1;
    uint8_t DISP_LCO : 1;
    }; uint8_t u8;
} t_tun_disp;

typedef union{
    struct{
    uint8_t RESERVED : 6;
    uint8_t CALIB_NOK : 1;
    uint8_t CALIB_DONE : 1;
    }; uint8_t u8;
} t_calib;

// direct command send to PRESET_DEFAULT and CALIB_RCO
#define DIRECT_COMMAND  (0x96)
// distance out of range
#define DIST_OUT_OF_RANGE   (0x3f)

int as3935_displco(uint8_t n);
int as3935_get_displco(uint8_t *n);
int as3935_tuncap(uint8_t n);
int as3935_get_tuncap(uint8_t *n);
int as3935_gain(uint8_t n);
int as3935_get_gain(uint8_t *n);
int as3935_wakeup();
int as3935_calib_rco();
//int as3935_set_gain(uint8_t g);
int as3935_wdthres(uint8_t t);
int as3935_get_wdthres(uint8_t *t);
int as3935_nflev(uint8_t l);
int as3935_get_nflev(uint8_t *l);
int as3935_srej(uint8_t s);
int as3935_get_srej(uint8_t *s);
int as3935_minnumlig(uint8_t n);
int as3935_get_minnumlig(uint8_t *n);
int as3935_clearstat();
int as3935_intcode(uint8_t *code);
int as3935_maskdist(uint8_t m);
int as3935_get_maskdist(uint8_t *m);
int as3935_lco_fdiv(uint8_t d);
int as3935_get_lco_fdiv(uint8_t *d);
int as3935_energy(uint32_t *E);
int as3935_distance(uint8_t *d);
int as3935_resetdef();
