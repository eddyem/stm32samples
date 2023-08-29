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

// macro for static strings
#define SEND(str) do{addtobuf(str);}while(0)

#ifdef EBUG
#define MSG(str)  do{addtobuf(__FILE__ " (L" STR(__LINE__) "): " str);}while(0)
#else
#define MSG(str)
#endif

#define newline() do{bufputchar('\n');}while(0)

extern uint8_t noLED;
void cmd_parser(char *buf, uint8_t isUSB);
void addtobuf(const char *txt);
void bufputchar(char ch);
void printu(uint32_t val);
void printuhex(uint32_t val);
void sendbuf();
char *getnum(char *txt, int32_t *N);
void mesg(char *txt);

#endif // __PROTO_H__
