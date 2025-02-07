/*
 * This file is part of the nitrogen project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "strfunc.h"
#include "usb.h"

#ifndef _U_
#define _U_ __attribute__((__unused__))
#endif

#define CMD_MAXLEN  (32)

enum{
   RET_WRONGPARNO = -3,     // wrong parameter number
   RET_CMDNOTFOUND = -2,    // command not found
   RET_WRONGARG = -1,       // wrong argument
   RET_GOOD = 0,            // all OK
   RET_BAD = 1              // something wrong
};

#define printu(x)       do{USB_sendstr(u2str(x));}while(0)
#define printi(x)       do{USB_sendstr(i2str(x));}while(0)
#define printuhex(x)    do{USB_sendstr(uhex2str(x));}while(0)
#define printf(x)       do{USB_sendstr(float2str(x, 2));}while(0)

extern uint8_t ShowMsgs; // show CAN messages flag

const char *cmd_parser(const char *txt);
