/*
 * This file is part of the TETRIS project.
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
#ifndef DEBUG_H__
#define DEBUG_H__

#ifdef EBUG
#include "usb.h"
#include "proto.h"
#define DBG(x)      USB_send(x)
#define DBGU(x)     USB_send(u2str(x))
#define NL()        USB_send("\n")
#else // EBUG
#define DBG(x)
#define DBGU(x)
#define NL()
#endif // EBUG

#endif // DEBUG_H__

