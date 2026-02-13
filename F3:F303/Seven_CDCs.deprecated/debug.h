/*
 * This file is part of the 7CDCs project.
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

#include "usb.h"

#ifdef EBUG
#define DBG(str)  do{USB_sendstr(DBG_IDX, __FILE__ " (L" STR(__LINE__) "): " str); newline(DBG_IDX);}while(0)
#define DBGmesg(str) do{USB_sendstr(DBG_IDX, str);}while(0)
#define DBGmesgn(str,n) do{USB_send(DBG_IDX, str, n);}while(0)
#define DBGnl()  newline(DBG_IDX)
#else
#define DBG(str)
#define DBGmesg(str)
#define DBGmesgn(str,n)
#define DBGnl()
#endif

#define MSG(str)  do{USB_sendstr(DBG_IDX, str); newline(DBG_IDX);}while(0)

