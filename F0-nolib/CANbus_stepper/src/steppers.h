/*
 * This file is part of the Stepper project.
 * Copyright 2020 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#ifndef STEPPERS_H__
#define STEPPERS_H__

#include <stm32f0.h>

// the lowest speed equal to [max speed] / LOWEST_SPEED_DIV
#define LOWEST_SPEED_DIV    30
// min/max value of ACCDECSTEPS
#define ACCDECSTEPS_MIN     30
#define ACCDECSTEPS_MAX     2000

typedef enum{
    DRV_NONE,   // driver is absent
    DRV_NOTINIT,// not initialized
    DRV_MAILF,  // mailfunction - no Vdd when Vio_ON activated
    DRV_8825,   // DRV8825 connected
    DRV_4988,   // A4988 connected
    DRV_2130,   // TMC2130 connected
    DRV_MAX     // amount of records in enum
} drv_type;

// stepper states
typedef enum{
    STP_SLEEP,      // don't moving
    STP_ACCEL,      // start moving with acceleration
    STP_MOVE,       // moving with constant speed
    STP_MVSLOW,     // moving with slowest constant speed
    STP_DECEL,      // moving with deceleration
    STP_STOP,       // stop motor right now (by demand)
    STP_STOPZERO,   // stop motor and zero its position (on end-switch)
    STP_MOVE0,      // move towards 0 endswitch (negative direction)
    STP_MOVE1,      // move towards 1 endswitch (positive direction)
} stp_state;

typedef enum{
    STPS_ALLOK,     // no errors
    STPS_ACTIVE,    // motor is still moving
    STPS_TOOBIG,    // amount of steps too big
    STPS_ZEROMOVE,  // give 0 steps to move
    STPS_ONESW      // staying on end-switch & try to move further
} stp_status;

extern int32_t mot_position;
extern uint32_t steps_left;
extern stp_state state;
#define stp_getstate()  (state)

drv_type initDriver();
drv_type getDrvType();
uint16_t getMaxUsteps();

void stp_chspd();
stp_status stp_move(int32_t steps);
void stp_stop();
void stp_process();
void stp_chARR(uint32_t val);

#endif // STEPPERS_H__





