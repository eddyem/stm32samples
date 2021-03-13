/*
 * This file is part of the MAX7219 project.
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
#ifndef HARDWARE_H__
#define HARDWARE_H__

#include "stm32f1.h"

// LED0 - PC13 (bluepill), blinking each second
#define LED0_port   GPIOC
#define LED0_pin    (1<<13)

// USB pullup (not present in bluepill, should be soldered) - PA15
#define USBPU_port  GPIOA
#define USBPU_pin   (1<<15)
#define USBPU_ON()  pin_set(USBPU_port, USBPU_pin)
#define USBPU_OFF() pin_clear(USBPU_port, USBPU_pin)

#define LED_blink(x)    pin_toggle(x ## _port, x ## _pin)
#define LED_on(x)       pin_clear(x ## _port, x ## _pin)
#define LED_off(x)      pin_set(x ## _port, x ## _pin)

// SPI DMA channel
#define DMA_SPI_Channel DMA1_Channel3
// SCREEN PINs: PA5 - SCK, PA6 - CS, PA7 - MOSI
#define CS_port     GPIOA
#define CS_pin      (1<<6)
#define SET(x)      pin_set(x ## _port, x ## _pin)
#define CLEAR(x)    pin_clear(x ## _port, x ## _pin)
#define TOGGLE(x)   pin_toggle(x ## _port, x ## _pin)

// amount of 8x8 matrices
#define NSCREENS    (4)

void hw_setup();
void iwdg_setup();

#endif // HARDWARE_H__
