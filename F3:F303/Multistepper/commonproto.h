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

#ifndef _U_
#define _U_         __attribute__((unused))
#endif

// message have no parameter
#define CANMESG_NOPAR       (127)
// message is setter (have value)
#define SETTERFLAG      (0x80)
#define ISSETTER(x)     ((x & SETTERFLAG) ? 1 : 0)
// base value of parameter (even if it is a setter)
#define PARBASE(x)      (x & 0x7f)

// error codes for answer message
typedef enum{
    ERR_OK,         // 0 - all OK
    ERR_BADPAR,     // 1 - parameter's value is wrong
    ERR_BADVAL,     // 2 - wrong parameter's value
    ERR_WRONGLEN,   // 3 - wrong message length
    ERR_BADCMD,     // 4 - unknown command
    ERR_CANTRUN,    // 5 - can't run given command due to bad parameters or other
    ERR_AMOUNT      // amount of error codes
} errcodes;

// pointer to function for command execution, both should be non-NULL for common cases
// if(par &0x80) it is setter, if not - getter
// if par == 0x127 it means absense of parameter!!!
// @return CANERR_OK (0) if OK or error code
typedef errcodes (*fpointer)(uint8_t par, int32_t *val);

enum{
     CCMD_NONE               // omit zero
    ,CCMD_PING               // ping device
    ,CCMD_RELAY              // relay on/off
    ,CCMD_BUZZER             // buzzer on/off
    ,CCMD_ADC                // ADC ch#
    ,CCMD_BUTTONS            // buttons
    ,CCMD_ESWSTATE           // end-switches state
    ,CCMD_MCUT               // MCU temperature
    ,CCMD_MCUVDD             // MCU Vdd
    ,CCMD_RESET              // software reset
    ,CCMD_TIMEFROMSTART      // get time from start
    ,CCMD_PWM                // PWM value
    ,CCMD_EXT                // value on EXTx outputs
    ,CCMD_SAVECONF           // save configuration
    ,CCMD_ENCSTEPMIN         // min ticks of encoder per one step
    ,CCMD_ENCSTEPMAX         // max ticks of encoder per one step
    ,CCMD_MICROSTEPS         // get/set microsteps
    ,CCMD_ACCEL              // set/get acceleration/deceleration
    ,CCMD_MAXSPEED           // set/get maximal speed
    ,CCMD_MINSPEED           // set/get minimal speed
    ,CCMD_SPEEDLIMIT         // get limit of speed for current microsteps settings
    ,CCMD_MAXSTEPS           // max steps (-max..+max)
    ,CCMD_ENCREV             // encoder's pulses per revolution
    ,CCMD_MOTFLAGS           // motor flags
    ,CCMD_ESWREACT           // ESW reaction flags
    ,CCMD_REINITMOTORS       // re-init motors after configuration changing
    ,CCMD_ABSPOS             // current position (set/get)
    ,CCMD_RELPOS             // set relative steps or get steps left
    ,CCMD_RELSLOW            // change relative position at lowest speed
    ,CCMD_EMERGSTOP          // stop moving NOW
    ,CCMD_STOP               // smooth motor stop
    ,CCMD_EMERGSTOPALL       // emergency stop for all motors
    ,CCMD_GOTOZERO           // go to zero's ESW
    ,CCMD_MOTORSTATE         // motor state
    ,CCMD_ENCPOS             // position of encoder (independing on settings)
    ,CCMD_SETPOS             // set motor position
    // should be the last:
    ,CCMD_AMOUNT             // amount of common commands
};

extern const fpointer cancmdlist[CCMD_AMOUNT];
extern const char* cancmds[CCMD_AMOUNT];

// all common functions
errcodes cu_ping(_U_ uint8_t par, _U_ int32_t *val);
// not realized yet
errcodes cu_abspos(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_accel(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_adc(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_button(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_canid(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_diagn(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_emstop(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_eraseflash(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_esw(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_eswreact(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_goto(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_gotoz(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_gpio(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_gpioconf(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_maxspeed(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_maxsteps(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_mcut(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_mcuvdd(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_microsteps(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_minspeed(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_motflags(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_motmul(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_motreinit(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_relpos(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_relslow(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_saveconf(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_screen(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_speedlimit(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_state(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_stop(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_tmcbus(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_udata(_U_ uint8_t par, _U_ int32_t *val);
errcodes cu_usartstatus(_U_ uint8_t par, _U_ int32_t *val);
