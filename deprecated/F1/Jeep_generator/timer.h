/*
 * timer.h
 *
 * Copyright 2016 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#ifndef __TIMER_H__
#define __TIMER_H__

#include "main.h"
#include "hardware_ini.h"

//~ // 180rpm - 120Hz, T/2=8333us
//~ #define TM2_MIN_SPEED (8333)
//~ // 6000rpm - 4kHz, T/2=250us
//~ #define TM2_MAX_SPEED (250)
// max & min rotation speed
#define MAX_RPM  (6000)
#define MIN_RPM  (200)


void tim2_init();
void increase_speed();
void decrease_speed();

extern uint16_t current_RPM;

#endif // __TIMER_H__
