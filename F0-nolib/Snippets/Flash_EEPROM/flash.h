/*
 *                                                                                                  geany_encoding=koi8-r
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

typedef struct{
    uint16_t userconf_sz; // size of data
    uint16_t devID; // device address (id)
    uint16_t ESW_thres; // ADC threshold for end-switches/Hall sensors
    // calibration values for current/voltage sensors
    uint16_t v12numerator; // 12V to motors
    uint16_t v12denominator;
    uint16_t i12numerator; // motors' current
    uint16_t i12denominator;
    uint16_t v33numerator; // 3.3V (vref)
    uint16_t v33denominator;
} user_conf;

extern user_conf the_conf;

void get_userconf();
int store_userconf();

#endif // __FLASH_H__
