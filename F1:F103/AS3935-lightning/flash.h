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
#include "hardware.h" // for SENSORS_AMOUNT

#define FLASH_SIZE_REG      ((uint32_t)0x1FFFF7E0)
#define FLASH_SIZE          *((uint16_t*)FLASH_SIZE_REG)

// maximal size (in letters) of iInterface for settings
#define MAX_IINTERFACE_SZ   (16)

typedef struct{
    // t_afe_gain
    uint8_t AFE_GB : 5;
    // t_threshold
    uint8_t WDTH : 4;
    uint8_t NF_LEV : 3;
    // t_lightning_reg
    uint8_t SREJ : 4;
    uint8_t MIN_NUM_LIG : 2;
    // t_int_mask_ant
    uint8_t MASK_DIST : 1;
    uint8_t LCO_FDIV : 2;
    // t_tun_disp
    uint8_t TUN_CAP : 4;
} sens_pars_t;

typedef struct{
    uint8_t restore : 1;   // restore sensors' parameters on start
} flags_t;

/*
 * struct to save user configurations
 */
typedef struct __attribute__((packed, aligned(4))){
    uint16_t userconf_sz;       // "magick number"
    uint16_t iInterface[MAX_IINTERFACE_SZ]; // interface name
    uint8_t iIlength;                       // length in symbols
    sens_pars_t spars[SENSORS_AMOUNT];      // sensors' stored flags
    flags_t flags;                          // common flags
} user_conf;

extern uint32_t maxCnum;
extern user_conf the_conf;
extern int currentconfidx;

void flashstorage_init();
int store_userconf();
int erase_storage(int npage);
