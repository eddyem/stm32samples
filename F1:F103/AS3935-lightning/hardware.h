/*
 * This file is part of the as3935 project.
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

#include <stm32f1.h>

// amount of connected sensors
#define SENSORS_AMOUNT  2

// LED0 - PC13 (bluepill), blinking each second
#define LED0_port       GPIOC
#define LED0_pin        (1<<13)

// USB pullup (not present in bluepill, should be soldered) - PA15
#define USBPU_port      GPIOA
#define USBPU_pin       (1<<15)
#define USBPU_ON()      pin_set(USBPU_port, USBPU_pin)
#define USBPU_OFF()     pin_clear(USBPU_port, USBPU_pin)

#define LED_blink(x)    pin_toggle(x ## _port, x ## _pin)
#define LED_on(x)       pin_clear(x ## _port, x ## _pin)
#define LED_off(x)      pin_set(x ## _port, x ## _pin)

// CS pins: PA2/PA3
#define CS_OFF()        do{GPIOA->BSRR = 0xc;}while(0)
#define CS(x)           do{GPIOA->BSRR = (x) ? (1<<19 | 1<<2) : (1<<18 | 1<<3);}while(0)

// INT pins: PA0/PA1
#define CHK_INT(x)      ((GPIOA->IDR & (1<<x)) ? 1 : 0)

enum{
    DISPLCO_NOTHING,
    DISPLCO_TRCO,
    DISPLCO_SRCO,
    DISPLCO_LCO
};

extern uint8_t DISPLCO[SENSORS_AMOUNT];

void hw_setup();
void iwdg_setup();
