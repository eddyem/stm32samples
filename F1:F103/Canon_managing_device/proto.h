/*
 * This file is part of the canonmanage project.
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
#ifndef PROTO_H__
#define PROTO_H__

#include <stm32f1.h>

#define printu(x)       do{USB_sendstr(u2str(x));}while(0)
#define printuhex(x)    do{USB_sendstr(uhex2str(x));}while(0)

#ifdef EBUG
#define DBG(x)      do{USB_send(x); USB_send("\n");}while(0)
#else
#define DBG(x)
#endif

void parse_cmd(const char *buf);
char *omit_spaces(const char *buf);
char *getnum(const char *buf, uint32_t *N);
char *u2str(uint32_t val);
char *u2hexstr(uint32_t val);

#endif // PROTO_H__
