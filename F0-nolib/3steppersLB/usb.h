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
#ifndef __USB_H__
#define __USB_H__

#include "hardware.h"

#define BUFFSIZE   (64)

// send string with constant length
#define USND(str)  do{USB_send((uint8_t*)str, sizeof(str)-1);}while(0)

void USB_setup();
void usb_proc();
void USB_send(const uint8_t *buf, uint16_t len);
void USB_sendstr(const char *str);
void USB_send_blk(const uint8_t *buf, uint16_t len);
uint8_t USB_receive(uint8_t *buf);

#endif // __USB_H__
