/*
 * This file is part of the SockFans project.
 * Copyright 2020 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#ifndef FLASH_H__
#define FLASH_H__

#include "hardware.h"

#define FLASH_BLOCK_SIZE    (1024)
#define FLASH_SIZE_REG      ((uint32_t)0x1FFFF7CC)
#define FLASH_SIZE          *((uint16_t*)FLASH_SIZE_REG)

#define TMINNO  (3)
#define TMAXNO  (4)

/*
 * struct to save user configurations
 */
typedef struct __attribute__((packed, aligned(4))){
    uint16_t userconf_sz;       // "magick number"
    uint32_t Tturnoff;          // wait for Tturnoff ms before turning power off
    uint32_t Thyst;             // Thysteresis
    int16_t Tmin[TMINNO];       // minT
    int16_t Tmax[TMAXNO];       // maxT
} user_conf;

extern user_conf the_conf; // global user config (read from FLASH to RAM)
// data from ld-file: start address of storage
extern const uint32_t __varsstart;

void flashstorage_init();
int store_userconf();
void dump_userconf();

#endif // FLASH_H__
