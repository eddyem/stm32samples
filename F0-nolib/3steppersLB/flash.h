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

// limiting values
#define MICROSTEPSMAX       (512)
// (STEPS per second^2)
#define ACCELMAXSTEPS       (1000)
// max encoder steps per rev
#define MAXENCREV           (100000)

// register with flash size (in blocks)
#ifndef FLASH_SIZE_REG
#define FLASH_SIZE_REG      ((uint32_t)0x1FFFF7CC)
#endif

#define FLASH_SIZE          *((uint16_t*)FLASH_SIZE_REG)

// motor flags
typedef struct{
    uint8_t reverse : 1;        // bit0 - reversing motor rotation
    uint8_t encreverse : 1;     // bit1 - reversing encoder rotation TODO: configure encoder's timer to downcounting
    uint8_t haveencoder : 1;    // bit2 - have encoder
    uint8_t donthold : 1;       // bit3 - clear power @ stop (don't hold motor when stopped)
    uint8_t eswinv : 1;         // bit4 - invers end-switches
    uint8_t keeppos : 1;        // bit5 - keep current position (as servo motor)
} motflags_t;

/*
 * struct to save user configurations
 */
typedef struct __attribute__((packed, aligned(4))){
    uint16_t userconf_sz;           // "magick number"
    uint16_t CANspeed;              // default CAN speed
    uint16_t CANID;                 // identifier
    uint16_t microsteps[MOTORSNO];  // microsteps amount per step
    uint16_t accel[MOTORSNO];       // acceleration/deceleration (steps/s^2)
    uint16_t maxspd[MOTORSNO];      // max motor speed (steps per second)
    uint16_t minspd[MOTORSNO];      // min motor speed (steps per second)
    uint32_t maxsteps[MOTORSNO];    // maximal amount of steps
    uint16_t encrev[MOTORSNO];      // encoders' counts per revolution
    uint16_t encperstepmin[MOTORSNO]; // min amount of encoder ticks per one step
    uint16_t encperstepmax[MOTORSNO]; // max amount of encoder ticks per one step
    motflags_t motflags[MOTORSNO];  // motor's flags
    uint8_t ESW_reaction[MOTORSNO]; // end-switches reaction (esw_react)
} user_conf;

extern user_conf the_conf; // global user config (read from FLASH to RAM)
// data from ld-file: start address of storage

void flashstorage_init();
int store_userconf();
void dump_userconf(_U_ char *txt);

#endif // __FLASH_H__
