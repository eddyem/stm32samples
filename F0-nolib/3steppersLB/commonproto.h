/*
 * This file is part of the 3steppers project.
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
#ifndef COMMONPROTO_H__
#define COMMONPROTO_H__

#include <stm32f0.h>

#include "can.h"

#define _U_         __attribute__((unused))

// message have no parameter
#define CANMESG_NOPAR       (127)
// message is setter (have value)
#define ISSETTER(x)     ((x & 0x80) ? 1 : 0)
// base value of parameter (even if it is a setter)
#define PARBASE(x)      (x & 0x7f)

// error codes for answer message
typedef enum{
    ERR_OK,         // all OK
    ERR_BADPAR,     // parameter's value is wrong
    ERR_BADVAL,     // wrong parameter's value
    ERR_WRONGLEN,   // wrong message length
    ERR_BADCMD,     // unknown command
    ERR_CANTRUN,    // can't run given command due to bad parameters or other
} errcodes;

// pointer to function for command execution, both should be non-NULL for common cases
// if(par &0x80) it is setter, if not - getter
// if par == 0x127 it means absense of parameter!!!
// @return CANERR_OK (0) if OK or error code
typedef errcodes (*fpointer)(uint8_t *par, int32_t *val);

typedef struct{
    const char *command;    // text command (up to 65536 commands)
    fpointer function;      // function to execute: function(&par, &val)
    const char *help;       // help message for text protocol
} commands;

enum{
     CMD_PING               // ping device
    ,CMD_RELAY              // relay on/off
    ,CMD_BUZZER             // buzzer on/off
    ,CMD_ADC                // ADC ch#
    ,CMD_BUTTONS            // buttons
    ,CMD_ESWSTATE           // end-switches state
    ,CMD_MCUT               // MCU temperature
    ,CMD_MCUVDD             // MCU Vdd
    ,CMD_RESET              // software reset
    ,CMD_TIMEFROMSTART      // get time from start
    ,CMD_PWM                // PWM value
    ,CMD_EXT                // value on EXTx outputs
    ,CMD_AMOUNT             // amount of CANBUS commands, pure USB commands coming after it
};

extern const commands cmdlist[CMD_AMOUNT];

#endif // COMMONPROTO_H__
