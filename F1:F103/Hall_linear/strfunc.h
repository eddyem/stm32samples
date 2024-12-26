/*
 * This file is part of the fx3u project.
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

#ifndef _U_
#define _U_ __attribute__((__unused__))
#endif

void hexdump(int (*sendfun)(const char *s), uint8_t *arr, uint16_t len);
char *u2str(uint32_t val);
char *i2str(int32_t i);
char *uhex2str(uint32_t val);
const char *getnum(const char *txt, uint32_t *N);
const char *omit_spaces(const char *buf);
const char *getint(const char *txt, int32_t *I);

#define EQ()        do{USB_putbyte('=');}while(0)
#define printu(a)   do{USB_sendstr(u2str(a));}while(0)
#define printi(a)   do{USB_sendstr(i2str(a));}while(0)
#define printuhex(a) do{USB_sendstr(uhex2str(a));}while(0)
