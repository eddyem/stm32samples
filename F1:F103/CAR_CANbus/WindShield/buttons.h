/*
 * This file is part of the windshield project.
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

#include <stm32f1.h>

// long press detector (>99ms)
#define KEY_HOLDTIME    (99)

// keycode for press/release
typedef enum{
    HALL_D,     // hall sensor DOWN
    HALL_U,     // -//- UP
    KEY_D,      // user key DOWN
    KEY_U,      // -//- UP
    DIR_D,      // ext signal (direct motor cables) DOWN
    DIR_U,      // -//- UP
    KEY_AMOUNT  // amount of keys connected
} keycode;

// events
typedef enum{
    EVT_NONE,   // no events with given key
    EVT_PRESS,  // pressed
    EVT_HOLD,   // hold more than KEY_HOLDTIME ms
    EVT_RELEASE // released
} keyevent;

typedef struct{
    keyevent event;         // current key event
    int16_t counter;        // press/release milliseconds counter
    GPIO_TypeDef *port;     // key port
    uint16_t pin;           // key pin
    uint8_t changed : 1;    // the event have been changed
    uint8_t inverted : 1;   // button inverted: 0 - passive, 1 - active
} keybase;

//extern uint32_t lastUnsleep; // last keys activity time

int process_keys();
void clear_events();
uint8_t keystate(keycode k, keyevent *evt);
keyevent keyevt(keycode k);
