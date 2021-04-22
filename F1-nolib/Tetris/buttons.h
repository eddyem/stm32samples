/*
 * This file is part of the TETRIS project.
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

#include <stm32f1.h>

// long press detector (>99ms)
#define KEY_HOLDTIME    (99)

// keycode for press/release
typedef enum{
    KEY_L,      // left
    KEY_R,      // right
    KEY_U,      // up (pause)
    KEY_D,      // down
    KEY_M,      // menu (rotate left)
    KEY_S,      // select (rotate right)
    KEY_AMOUNT  // amount of keys connected
} keycode;

// events
typedef enum{
    EVT_NONE,   // no events with given key
    EVT_PRESS,  // pressed
    EVT_HOLD,   // hold more than KEY_HOLDTIME ms
    EVT_RELEASE // released
} keyevent;

void process_keys();
void clear_events();
uint8_t keystate(keycode k, keyevent *evt);
keyevent keyevt(keycode k);

#define BUTTONS_H__
#endif // BUTTONS_H__
