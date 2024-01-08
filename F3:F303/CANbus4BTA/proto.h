/*
 * This file is part of the canbus4bta project.
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

/**
 * USB-only and common (CAN/USB) protocol functions parser
 */

#pragma once

#include <stm32f3.h>

#ifndef _U_
#define _U_ __attribute__((__unused__))
#endif

// command parameter flag means this is a setter
#define SETTER_FLAG     (0x80)
#define ISSETTER(data)  ((data[2] & SETTER_FLAG))
// parameter number 127 means there no parameter number at all (don't need paremeter or get all)
#define NO_PARNO        (0x1f)

// make error for CAN answer
#define FORMERR(m, err) do{m->data[3] = err; if(m->length < 4) m->length = 4;}while(0)

// error codes for answer message
typedef enum{
    ERR_OK,         // 0 - all OK
    ERR_BADPAR,     // 1 - parameter number is wrong
    ERR_BADVAL,     // 2 - wrong parameter's value
    ERR_WRONGLEN,   // 3 - wrong message length
    ERR_BADCMD,     // 4 - unknown command
    ERR_CANTRUN,    // 5 - can't run given command due to bad parameters or other problems
    ERR_AMOUNT      // amount of error codes
} errcodes;

// CAN commands indexes
typedef enum{
    CMD_RESET,      // 0 - reset MCU
    CMD_TIME,       // 1 - get/set Tms
    CMD_MCUTEMP,    // 2 - get MCU temperature (*10)
    CMD_ADCRAW,     // 3 - get ADC raw values
    CMD_ADCV,       // 4 - get ADC voltage (*100)
    CMD_CANSPEED,   // 5 - get/set CAN speed (kbps)
    CMD_CANID,      // 6 - get/set CAN ID
    CMD_ADCMUL,     // 7 - get/set ADC multipliers 0..4
    CMD_SAVECONF,   // 8 - save configuration
    CMD_ERASESTOR,  // 9 - erase all flash storage
    CMD_RELAY,      // 10 - switch relay ON/OFF
    CMD_GETESW_BLK, // 11 - blocking read of ESW
    CMD_GETESW,     // 12 - current ESW state, bounce-free
    CMD_BOUNCE,     // 13 - get/set bounce constant (ms)
    CMD_AMOUNT      // amount of CAN commands
} can_cmd;

