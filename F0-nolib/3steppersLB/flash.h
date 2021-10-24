/*
 * flash.h
 *
 * Copyright 2017 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#pragma once
#ifndef __FLASH_H__
#define __FLASH_H__

#include "hardware.h"

#ifndef _U_
#define _U_         __attribute__((unused))
#endif


// register with flash size (in blocks)
#ifndef FLASH_SIZE_REG
blocksizeASH_SIZE_REG      ((uint32_t)0x1FFFF7CC)
#endif

#define FLASH_SIZE          *((uint16_t*)FLASH_SIZE_REG)

// motor flags
typedef struct{
    uint8_t reverse : 1;
} motflags_t;

/*
 * struct to save user configurations
 */
typedef struct __attribute__((packed, aligned(4))){
    uint16_t userconf_sz;           // "magick number"
    uint16_t CANspeed;              // default CAN speed
    uint16_t CANID;                 // identifier
    uint16_t microsteps[MOTORSNO];  // microsteps amount per step
    uint16_t accdecsteps[MOTORSNO]; // amount of steps need for full acceleration/deceleration cycle
    uint16_t maxspd[MOTORSNO];      // max motor speed (steps per second)
    uint32_t maxsteps[MOTORSNO];    // maximal amount of steps from ESW0 to EWS3
    motflags_t motflags[MOTORSNO];  // motor's flags
} user_conf;

extern user_conf the_conf; // global user config (read from FLASH to RAM)
// data from ld-file: start address of storage

void flashstorage_init();
int store_userconf();
void dump_userconf(_U_ char *txt);

#endif // __FLASH_H__
