/*
 *                                                                                                  geany_encoding=koi8-r
 * sensors_manage.h
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#pragma once
#ifndef __SENSORS_MANAGE_H__
#define __SENSORS_MANAGE_H__

#include "hardware.h"

// time for power up procedure (500ms)
#define POWERUP_TIME        (500)
// time between readings in scan mode (15sec)
#define SLEEP_TIME          (15000)
// error in measurement == -300degrC
#define BAD_TEMPERATURE     (-30000)
// no sensor on given channel
#define NO_SENSOR           (-31000)

typedef enum{
     SENS_INITING           // 0 power on
    ,SENS_RESETING          // 1 discovery sensors resetting them
    ,SENS_GET_COEFFS        // 2 get coefficients from all sensors
    ,SENS_SLEEPING          // 3 wait for a time to process measurements
    ,SENS_START_MSRMNT      // 4 send command 2 start measurement
    ,SENS_WAITING           // 5 wait for measurements end
    ,SENS_GATHERING         // 6 collect information
    ,SENS_OFF               // 7 sensors' power is off by external command
    ,SENS_OVERCURNT         // 8 overcurrent detected @ any stage
    ,SENS_OVERCURNT_OFF     // 9 sensors' power is off due to continuous overcurrent
    ,SENS_SENDING_DATA      // A send data over CAN bus
    ,SENS_STATE_CNT
} SensorsState;

extern uint8_t sensors_scan_mode;
extern int16_t Temperatures[MUL_MAX_ADDRESS+1][2];
extern uint8_t sens_present[2];
extern SensorsState Sstate;
extern uint8_t Nsens_present;
extern uint8_t Ntemp_measured;

const char *sensors_get_statename(SensorsState x);
void sensors_process();

void sensors_off();
void sensors_init();
void sensors_start();
void showcoeffs();
void showtemperature();

#endif // __SENSORS_MANAGE_H__
