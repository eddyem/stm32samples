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

#include <stdint.h>
#include "usb_descr.h"

// register with flash size (in blocks)
#ifndef FLASH_SIZE_REG
#define FLASH_SIZE_REG      ((uint32_t)0x1FFFF7CC)
#endif

#define FLASH_SIZE          *((uint16_t*)FLASH_SIZE_REG)

// maximal size (in letters, ASCII, no ending \0) of iInterface for settings
#define MAX_IINTERFACE_SZ   (16)

/*
 * struct to save user configurations
 */
typedef struct __attribute__((packed, aligned(4))){
    uint16_t userconf_sz;           // "magick number"
    uint16_t CANspeed;              // default CAN speed (in kBaud!!!)
    uint16_t iInterface[InterfacesAmount][MAX_IINTERFACE_SZ]; // we store Interface name here in UTF!
    uint8_t  iIlengths[InterfacesAmount]; // length in BYTES (symbols amount x2)!
} user_conf;

extern user_conf the_conf; // global user config (read from FLASH to RAM)
extern int currentconfidx;

// data from ld-file: start address of storage

void flashstorage_init();
int store_userconf();
void dump_userconf();
int erase_storage();
