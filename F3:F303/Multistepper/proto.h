/*
 * This file is part of the multistepper project.
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

#define printu(x)       do{USB_sendstr(u2str(x));}while(0)
#define printi(x)       do{USB_sendstr(i2str(x));}while(0)
#define printuhex(x)    do{USB_sendstr(uhex2str(x));}while(0)
#define printf(x)       do{USB_sendstr(float2str(x, 2));}while(0)

extern uint8_t ShowMsgs; // show CAN messages flag

const char *cmd_parser(const char *txt);
uint8_t isgood(uint16_t ID);
