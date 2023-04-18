/*
 * This file is part of the SevenCDCs project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include <stm32f3.h>

#define USBPU_port  GPIOA
#define USBPU_pin   (1<<15)
#define USBPU_ON()  pin_clear(USBPU_port, USBPU_pin)
#define USBPU_OFF() pin_set(USBPU_port, USBPU_pin)

extern volatile uint32_t Tms;

void hw_setup();

#endif // __HARDWARE_H__
