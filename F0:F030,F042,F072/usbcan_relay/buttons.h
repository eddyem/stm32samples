/*
 * This file is part of the canrelay project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#ifndef BUTTONS_H__

#include <stm32f0.h>

// threshold in ms for press/hold
#define PRESSTHRESHOLD  (9)
#define HOLDTHRESHOLD   (199)

// events
typedef enum{
    EVT_NONE,   // no events with given key
    EVT_PRESS,  // pressed (hold more than PRESSTHRESHOLD ms)
    EVT_HOLD,   // hold more than HOLDTHRESHOLD ms
    EVT_RELEASE // released after press or hold state
} keyevent;

extern uint32_t lastUnsleep; // last keys activity time

void process_keys();
keyevent keystate(uint8_t k, uint32_t *T);
keyevent keyevt(uint8_t k);

#define BUTTONS_H__
#endif // BUTTONS_H__
