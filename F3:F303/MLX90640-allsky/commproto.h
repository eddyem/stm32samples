/*
 * This file is part of the as3935 project.
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

#include "mlx90640.h"

#include "version.inc"

#ifdef EBUG
#define RLSDBG "debug"
#else
#define RLSDBG "release"
#endif

#define REPOURL     "https://github.com/eddyem/stm32samples/tree/master/F3:F303/MLX90640-allsky "  RLSDBG " build #" BUILD_NUMBER "@" BUILD_DATE "\n"


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
// maximal available parameter number (for 16-bit registers is 0xffff
#define MAXPARNO    0xffff

typedef int (*sendfun_t)(const char*);

extern const char *EQ;
const char *parse_cmd(sendfun_t sendfun, char *buf);
void set_senders(int (*usbs)(const char*), int (*usbb)(uint8_t), int (*usbbin)(const uint8_t*, int),
                 int (*usarts)(const char*), int (*usartb)(uint8_t),  int (*usartbin)(const uint8_t*, int));
void set_sender(sendfun_t sendfun);
sendfun_t get_sender();

extern const char *const Timage;

extern uint8_t cartoon;
void dumpIma(const fp_t im[MLX_PIXNO]);
void drawIma(const fp_t im[MLX_PIXNO]);
