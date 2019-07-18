/*
 * GPS.h
 *
 * Copyright 2015 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 */

#pragma once
#ifndef __GPS_H__
#define __GPS_H__

#include "stm32f1.h"

extern int need2startseq;

typedef enum{
     GPS_NOTFOUND   // default status before first RMC message
    ,GPS_WAIT       // wait for satellites
    ,GPS_NOT_VALID  // time known, but not valid
    ,GPS_VALID
} gps_status;

extern gps_status GPS_status;

void GPS_parse_answer(const char *string);
void GPS_send_start_seq();

#endif // __GPS_H__
