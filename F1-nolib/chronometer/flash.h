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

#include <stm32f1.h>
#include "hardware.h"

#define FLASH_BLOCK_SIZE    (1024)
#define FLASH_SIZE_REG      ((uint32_t)0x1FFFF7E0)
#define FLASH_SIZE          *((uint16_t*)FLASH_SIZE_REG)

/*
 * struct to save user configurations
 */
typedef struct __attribute__((packed, aligned(4))){
    uint16_t userconf_sz;       // "magick number"
    uint16_t NLfreeWarn;        // warn user when there's less free log records than NLfreeWarn
    uint8_t  trigstate;         // level in `triggered` state
    uint8_t  defflags;          // default flags
    uint16_t dist_min;          // minimal distance for LIDAR
    uint16_t dist_max;          // maximal -//-
    uint32_t USART_speed;       // USART1 speed (115200 by default)
    uint32_t LIDAR_speed;       // USART3 speed (115200 by default)
    uint16_t trigpause[TRIGGERS_AMOUNT]; // pause (ms) for false shots
} user_conf;

// values for user_conf.defflags:
// save events in flash
#define FLAG_SAVE_EVENTS        (1 << 0)
// strings ends with "\r\n" instead of normal "\n"
#define FLAG_STRENDRN           (1 << 1)
// proxy GPS messages over USART1
#define FLAG_GPSPROXY           (1 << 2)
// USART3 works as regular TTY instead of LIDAR
#define FLAG_NOLIDAR            (1 << 3)

/*
 * struct to save events logs
 */
typedef struct __attribute__((packed, aligned(4))){
    uint16_t elog_sz;
    uint8_t trigno;
    trigtime shottime;
    int16_t triglen;
    uint16_t lidar_dist;
} event_log;

extern user_conf the_conf;
extern const user_conf *Flash_Data;
extern const event_log *logsstart;
extern uint32_t maxCnum, maxLnum;
// data from ld-file
extern uint32_t _varslen, __varsstart, __logsstart;


void flashstorage_init();
int store_userconf();
int store_log(event_log *L);
int dump_log(int start, int Nlogs);

#ifdef EBUG
void dump_userconf();
void addNrecs(int N);
#endif

#endif // __FLASH_H__
