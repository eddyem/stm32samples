/*
 * This file is part of the F0testbrd project.
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
#ifndef PROTO_H__
#define PROTO_H__

#include "stm32f0.h"

#define USND(str)  do{USB_send((uint8_t*)str, sizeof(str)-1);}while(0)

extern volatile uint8_t ADCmon;

void USB_sendstr(const char *str);
char *get_USB();
const char *parse_cmd(char *buf);

void printADCvals();

char *u2str(uint32_t val);
char *i2str(int32_t i);
char *uhex2str(uint32_t val);
char *getnum(const char *txt, uint32_t *N);
char *omit_spaces(const char *buf);

#endif // PROTO_H__
