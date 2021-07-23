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
// newline with buffer sending over USART
#define NL() do{bufputchar('\n'); sendbuf();}while(0)

#define IGN_SIZE 10
extern uint16_t Ignore_IDs[IGN_SIZE];
extern uint8_t IgnSz;
extern uint8_t ShowMsgs;

void cmd_parser(char *buf);
void addtobuf(const char *txt);
void bufputchar(char ch);
void printu(uint32_t val);
void printuhex(uint32_t val);
void sendbuf();

char *omit_spaces(const char *buf);
char *getnum(const char *buf, uint32_t *N);

uint8_t isgood(uint16_t ID);

#endif // __PROTO_H__
