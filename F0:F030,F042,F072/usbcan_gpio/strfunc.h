/*
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

#ifndef USBIF
#error "Define USBIF to used interface before including this!"
#endif

#include <stdint.h>
#include <string.h>
#include "usb_dev.h"
#include "usb_descr.h"
#include "version.inc"

// DEBUG/RELEASE build
#ifdef EBUG
#define RLSDBG "debug"
#else
#define RLSDBG "release"
#endif

#define REPOURL "https://github.com/eddyem/stm32samples/tree/master/F0:F030,F042,F072/usbcan_gpio " RLSDBG " build #" BUILD_NUMBER "@" BUILD_DATE "\n"

// max string len to '\n' (including '\0')
#define MAXSTRLEN    256

void hexdump(int (*sendfun)(const char *s), uint8_t *arr, uint16_t len);
const char *u2str(uint32_t val);
const char *i2str(int32_t i);
const char *uhex2str(uint32_t val);
char *getnum(const char *txt, uint32_t *N);
char *omit_spaces(const char *buf);
char *getint(const char *txt, int32_t *I);

#define printu(u)       USB_sendstr(USBIF, u2str(u))
#define printuhex(u)    USB_sendstr(USBIF, uhex2str(u))

