/*
 * This file is part of the usbcanrb project.
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

#include <stm32f0.h>
#include "hardware.h"

#define BUFSZ   (64)

#ifdef EBUG
#define DBG(str)  do{USB_sendstr(__FILE__ " (L" STR(__LINE__) "): " str);}while(0)
#else
#define DBG(str)
#endif

#define IGN_SIZE 10
extern uint16_t Ignore_IDs[IGN_SIZE];
extern uint8_t IgnSz;
extern uint8_t ShowMsgs;

void cmd_parser(char *buf);
void printu(uint32_t val);
void printuhex(uint32_t val);

char *omit_spaces(const char *buf);
char *getnum(const char *buf, uint32_t *N);

uint8_t isgood(uint16_t ID);
