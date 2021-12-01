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

#ifndef _U_
#define _U_         __attribute__((unused))
#endif

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
typedef errcodes (*fpointer)(uint8_t par, int32_t *val);

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
    ,CMD_SAVECONF           // save configuration
    ,CMD_ENCSTEPMIN         // min ticks of encoder per one step
    ,CMD_ENCSTEPMAX         // max ticks of encoder per one step
    ,CMD_MICROSTEPS         // get/set microsteps
    ,CMD_ACCEL              // set/get acceleration/deceleration
    ,CMD_MAXSPEED           // set/get maximal speed
    ,CMD_MINSPEED           // set/get minimal speed
    ,CMD_SPEEDLIMIT         // get limit of speed for current microsteps settings
    ,CMD_MAXSTEPS           // max steps (-max..+max)
    ,CMD_ENCREV             // encoder's pulses per revolution
    ,CMD_MOTFLAGS           // motor flags
    ,CMD_ESWREACT           // ESW reaction flags
    ,CMD_REINITMOTORS       // re-init motors after configuration changing
    ,CMD_ABSPOS             // current position (set/get)
    ,CMD_RELPOS             // set relative steps or get steps left
    ,CMD_RELSLOW            // change relative position at lowest speed
    ,CMD_EMERGSTOP          // stop moving NOW
    ,CMD_STOP               // smooth motor stop
    ,CMD_EMERGSTOPALL       // emergency stop for all motors
    ,CMD_GOTOZERO           // go to zero's ESW
    ,CMD_MOTORSTATE         // motor state
    ,CMD_ENCPOS             // position of encoder (independing on settings)
    //,CMD_STOPDECEL
    //,CMD_FINDZERO
    // should be the last:
    ,CMD_AMOUNT             // amount of common commands
};

extern const fpointer cmdlist[CMD_AMOUNT];

#endif // COMMONPROTO_H__
