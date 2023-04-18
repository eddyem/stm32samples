/*
 * This file is part of the SevenCDCs project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#ifndef __USART_H__
#define __USART_H__

#include "hardware.h"
#include "usb_lib.h"

// amount of usarts
#define USARTSNO        3

extern volatile int linerdy, bufovr;

void usarts_setup();
void usart_config(uint8_t usartNo, usb_LineCoding *lc);
void usart_sendn(const uint8_t *str, int L);
usb_LineCoding *getLineCoding(int usartNo);

#endif // __USART_H__
