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

#include "adc.h"
#include "hardware.h"

// register with flash size (in blocks)
#ifndef FLASH_SIZE_BASE
#define FLASH_SIZE_BASE     ((uint32_t)0x1FFFF7CC)
#endif

#define FLASH_SIZE          *((uint16_t*)FLASH_SIZE_BASE)

/*
 * struct to save user configurations
 */
typedef struct __attribute__((packed, aligned(4))){
    uint16_t userconf_sz;           // "magick number"
    uint16_t CANID;                 // identifier
    uint32_t CANspeed;              // default CAN speed
    uint32_t bounce_ms;              // a
    float adcmul[ADC_TSENS];        // ADC voltage multipliers
} user_conf;

extern user_conf the_conf; // global user config (read from FLASH to RAM)
// these variables/constants need for dumpconf()
extern const user_conf *Flash_Data;
extern const uint32_t FLASH_blocksize;
extern int currentconfidx;

void flashstorage_init();
int store_userconf();
int erase_storage(int npage);
