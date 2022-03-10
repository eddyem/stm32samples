/*
 *                                                                                                  geany_encoding=koi8-r
 * proto.h
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#pragma once
#ifndef __PROTO_H__
#define __PROTO_H__

#include "stm32f0.h"
#include "hardware.h"

#define BUFSZ   (64)

// macro for static strings
#define SEND(str) do{addtobuf(str);}while(0)

#ifdef EBUG
#define MSG(str)  do{addtobuf(__FILE__ " (L" STR(__LINE__) "): " str);}while(0)
#else
#define MSG(str)
#endif

#define newline() do{bufputchar('\n');}while(0)
// newline with buffer sending
#define NL() do{bufputchar('\n'); sendbuf();}while(0)

extern uint8_t showMon;

void cmd_parser(char *buf);
void addtobuf(const char *txt);
void bufputchar(char ch);
void gett(char chno);
void printu(uint32_t val);
void printi(int32_t val);
void printuhex(uint32_t val);
void sendbuf();
void showState();

char *omit_spaces(char *buf);
char *getnum(char *buf, uint32_t *N);

#endif // __PROTO_H__
