/*
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

#include <stdint.h>

#include "usb_dev.h"

extern volatile uint32_t Tms;

#define STR_HELPER(s)   #s
#define STR(s)          STR_HELPER(s)

#ifdef EBUG
#define DBG(str)  do{USB_sendstr(__FILE__ " (L" STR(__LINE__) "): " str); newline();}while(0)
#else
#define DBG(str)
#endif

#define printu(x)       do{USB_sendstr(u2str(x));}while(0)
#define printi(x)       do{USB_sendstr(i2str(x));}while(0)
#define printuhex(x)    do{USB_sendstr(uhex2str(x));}while(0)
#define printfl(x,n)       do{USB_sendstr(float2str(x, n));}while(0)

void hexdump(int ifno, uint8_t *arr, uint16_t len);
const char *u2str(uint32_t val);
const char *i2str(int32_t i);
const char *uhex2str(uint32_t val);
const char *getnum(const char *txt, uint32_t *N);
const char *omit_spaces(const char *buf);
const char *getint(const char *txt, int32_t *I);
char *float2str(float x, uint8_t prec);
