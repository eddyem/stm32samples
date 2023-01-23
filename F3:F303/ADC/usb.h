/*
 * This file is part of the adc project.
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

#include "usbhw.h"

// sizes of ringbuffers for outgoing and incoming data
#define RBOUTSZ     (512)
#define RBINSZ      (512)

#define newline() USB_putbyte('\n')

void USB_proc();
int USB_sendall();
int USB_send(const uint8_t *buf, int len);
int USB_putbyte(uint8_t byte);
int USB_sendstr(const char *string);
int USB_receive(uint8_t *buf, int len);
int USB_receivestr(char *buf, int len);
