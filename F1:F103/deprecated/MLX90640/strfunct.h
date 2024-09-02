/*
 * This file is part of the MLX90640 project.
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
#ifndef STRFUNCT_H__
#define STRFUNCT_H__

#include "stm32f1.h"

#ifndef DENORM
#define DENORM  (0x007FFFFF)
#endif
#ifndef NAN
#define NAN     (0x7FC00000)
#endif
#ifndef INF
#define INF     (0x7F800000)
#endif
#ifndef MINF
#define MINF    (0xFF800000)
#endif

#define OBUFSZ      (64)
#define IBUFSZ      (256)

// macro for static strings
#define SEND(str) do{addtobuf(str);}while(0)

#ifdef EBUG
#define x__(a) #a
#define STR(a) x__(a)
#define DBG(str)  do{addtobuf(__FILE__ " (L" STR(__LINE__) "): " str); NL();}while(0)
#else
#define DBG(str)
#endif

#define newline() do{bufputchar('\n');}while(0)
// newline &  send buffer
#define NL() do{bufputchar('\n'); sendbuf();}while(0)

char *get_USB();
void addtobuf(const char *txt);
void bufputchar(char ch);
void printu(uint32_t val);
void printi(int32_t val);
void printuhex(uint32_t val);
void sendbuf();
char *omit_spaces(char *buf);
char *getnum(char *buf, int32_t *N);
void float2str(float x, uint8_t prec);

#endif // STRFUNCT_H__
