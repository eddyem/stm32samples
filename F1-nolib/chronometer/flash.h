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

#define FLASH_BLOCK_SIZE    (1024)
#define FLASH_SIZE_REG      ((uint32_t)0x1FFFF7E0)
#define FLASH_SIZE          *((uint16_t*)FLASH_SIZE_REG)

typedef struct{
    uint16_t userconf_sz;       // "magick number"
    uint32_t dist_min;          // minimal distance for LIDAR
    uint32_t dist_max;          // maximal -//-
} user_conf;

extern user_conf the_conf;

void get_userconf();
int store_userconf();
#ifdef EBUG
void dump_userconf();
void addNrecs(int N);
#endif

#endif // __FLASH_H__
