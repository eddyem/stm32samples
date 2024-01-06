/*
 * This file is part of the canbus4bta project.
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

#include "ringbuffer.h"
#include "usbhw.h"

// sizes of ringbuffers for outgoing and incoming data
#define RBOUTSZ     (512)
#define RBINSZ      (512)

#define newline()   USB_putbyte('\n')
#define USND(s)     do{USB_sendstr(s); USB_putbyte('\n');}while(0)

#define STR_HELPER(s)   #s
#define STR(s)          STR_HELPER(s)

#ifdef EBUG
#define DBG(str)  do{USB_sendstr(__FILE__ " (L" STR(__LINE__) "): " str); newline();}while(0)
#else
#define DBG(str)
#endif

extern volatile ringbuffer rbout, rbin;
extern volatile uint8_t bufisempty, bufovrfl;

void send_next();
int USB_sendall();
int USB_send(const uint8_t *buf, int len);
int USB_putbyte(uint8_t byte);
int USB_sendstr(const char *string);
int USB_receive(uint8_t *buf, int len);
int USB_receivestr(char *buf, int len);

#ifdef EBUG
#include "strfunc.h"
#define printu(x)       do{USB_sendstr(u2str(x));}while(0)
#define printi(x)       do{USB_sendstr(i2str(x));}while(0)
#define printuhex(x)    do{USB_sendstr(uhex2str(x));}while(0)
#define printf(x)       do{USB_sendstr(float2str(x, 2));}while(0)
#endif
