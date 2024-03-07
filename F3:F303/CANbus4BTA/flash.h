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

// bit flags positions
// encoder have SSI (1) or RS-422 (0)
#define ENC_IS_SSI_BIT      0

// bit number
#define FLAGBIT(f)      (f ## _BIT)
// flag position
#define FLAGP(f)        (1 << FLAGBIT(f))
// flag value
#define FLAG(f)         ((the_conf.flags & FLAGP(f)) ? 1 : 0)
// set flag
#define FLAGS(f)        do{the_conf.flags |= FLAGP(f);}while(0)
// reset flag
#define FLAGR(f)        do{the_conf.flags &= ~FLAGP(f);}while(0)

/*
 * struct to save user configurations
 */
typedef struct __attribute__((packed, aligned(4))){
    uint16_t userconf_sz;           // "magick number"
    uint16_t CANID;                 // identifier
    uint32_t CANspeed;              // default CAN speed
    uint32_t bounce_ms;             // debounce wait
    float adcmul[ADC_TSENS];        // ADC voltage multipliers
    uint32_t usartspeed;            // USART1 speed (baud/s)
    uint32_t flags;                 // bit flags
} user_conf;

extern user_conf the_conf; // global user config (read from FLASH to RAM)
// these variables/constants need for dumpconf()
extern const user_conf *Flash_Data;
extern const uint32_t FLASH_blocksize;
extern int currentconfidx;

void flashstorage_init();
int store_userconf();
int erase_storage(int npage);
