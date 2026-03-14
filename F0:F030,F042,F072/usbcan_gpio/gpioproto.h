/*
 * This file is part of the usbcangpio project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

// error codes for answer message
typedef enum{
    ERR_OK,         // all OK
    ERR_BADCMD,     // wrong command
    ERR_BADPAR,     // wrong parameter
    ERR_BADVAL,     // wrong value (for setter)
    ERR_WRONGLEN,   // wrong message length
    ERR_CANTRUN,    // can't run given command due to bad parameters or other
    ERR_BUSY,       // target interface busy, try later
    ERR_OVERFLOW,   // string was too long -> overflow
    ERR_AMOUNT      // amount of error codes or "send nothing"
} errcodes_t;

// maximal length of command (without trailing zero)
#define CMD_MAXLEN  15
// maximal available parameter number
#define MAXPARNO    255

// default threshold for monitoring
#define ADC_THRES_DEFAULT   100

void GPIO_process();
void GPIO_init();
