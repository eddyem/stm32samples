/*
 * This file is part of the encoders project.
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
#include "usb_descr.h"

#define FLASH_SIZE_REG      ((uint32_t)0x1FFFF7E0)
#define FLASH_SIZE          *((uint16_t*)FLASH_SIZE_REG)

// maximal size (in letters) of iInterface for settings
#define MAX_IINTERFACE_SZ   (16)

// min/max zeros before preambule
#define MINZEROS_MIN    2
#define MINZEROS_MAX    60
#define MAXZEROS_MIN    4
#define MAXZEROS_MAX    120

typedef struct{
    uint8_t CPOL : 1;
    uint8_t CPHA : 1;
    uint8_t BR   : 3;
    uint8_t monit: 1; // auto monitoring of encoder each `monittime` milliseconds
} flags_t;

/*
 * struct to save user configurations
 */
typedef struct __attribute__((packed, aligned(4))){
    uint16_t userconf_sz;       // "magick number"
    uint16_t iInterface[bTotNumEndpoints][MAX_IINTERFACE_SZ]; // hryunikod!
    uint8_t iIlengths[bTotNumEndpoints];
    uint8_t encbits;    // encoder bits: 26 or 32
    uint8_t encbufsz;   // encoder buffer size (up to ENCODER_BUFSZ_MAX)
    uint8_t minzeros;   // min/max zeros in preamble when searching start of record
    uint8_t maxzeros;
    uint8_t monittime;  // auto monitoring period (ms)
    flags_t flags;      // flags: CPOL, CPHA etc
} user_conf;

extern user_conf the_conf;
extern int currentconfidx;

void flashstorage_init();
int store_userconf();
int erase_storage(int npage);


