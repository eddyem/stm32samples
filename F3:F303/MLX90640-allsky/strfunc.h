/*
 * This file is part of the ir-allsky project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include <string.h>

#include "usb_dev.h"

#define printu(x)       do{USB_sendstr(u2str(x));}while(0)
#define printi(x)       do{USB_sendstr(i2str(x));}while(0)
#define printuhex(x)    do{USB_sendstr(uhex2str(x));}while(0)
#define printfl(x,n)    do{USB_sendstr(float2str(x, n));}while(0)

void u16s(uint16_t n, char *buf);
void hexdump16(int (*sendfun)(const char *s), uint16_t *arr, uint16_t len);
void hexdump(int (*sendfun)(const char *s), uint8_t *arr, uint16_t len);
const char *u2str(uint32_t val);
const char *i2str(int32_t i);
const char *uhex2str(uint32_t val);
const char *getnum(const char *txt, uint32_t *N);
char *omit_spaces(const char *buf);
const char *getint(const char *txt, int32_t *I);
char *float2str(float x, uint8_t prec);
