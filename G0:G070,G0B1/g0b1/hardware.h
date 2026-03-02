/*
 * This file is part of the test project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "stm32g0.h"

// KEY (intpullup->0) - PC13
// LED - PC6
#define KEY_PORT        GPIOC
#define KEY_PIN         (1<<13)
#define LED_PORT        GPIOC
#define LED_PIN         (1<<6)
#define KEY_PRESSED()   (pin_read(KEY_PORT, KEY_PIN) == 1)
#define LED_ON()        do{pin_set(LED_PORT, LED_PIN);}while(0)
#define LED_OFF()       do{pin_clear(LED_PORT, LED_PIN);}while(0)
#define LED_TOGGLE()    do{pin_toggle(LED_PORT, LED_PIN);}while(0)

extern volatile uint32_t Tms;
void gpio_setup();
