/*
 *                                                                                                  geany_encoding=koi8-r
 * can_process.h
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
#include "can.h"

// timeout for trying to send data
#define SEND_TIMEOUT_MS     (10)
// mark first byte if command sent
#define COMMAND_MARK    (0xA5)
// mark first byte if data sent
#define DATA_MARK       (0x5A)

// 8-bit commands sent by master
typedef enum{
    CMD_PING,               // request for PONG cmd
    CMD_START_MEASUREMENT,  // start thermal measurement (and turn ON sensors if was OFF)
    CMD_SENSORS_STATE,      // reply data with sensors state (data: 0 - SState, 1,2 - sens_present0, 3 - Nsens_presend, 4 - Ntemp_measured)
    CMD_START_SCAN,         // run scan mode @ all controllers
    CMD_STOP_SCAN,          // stop scan mode
    CMD_SENSORS_OFF,        // turn off power of sensors
    CMD_LOWEST_SPEED,       // lowest I2C speed
    CMD_LOW_SPEED,          // low I2C speed (10kHz)
    CMD_HIGH_SPEED,         // high I2C speed (100kHz)
    CMD_REINIT_I2C,         // reinit I2C with current speed
    CMD_CHANGE_MASTER_B,    // change master id to broadcast
    CMD_CHANGE_MASTER,      // change master id to 0
    CMD_GETMCUTEMP,         // MCU temperature value
    CMD_GETUIVAL,           // request to get values of V12, V5, I12 and V3.3
    CMD_GETUIVAL0,          // answer with values of V12 and V5
    CMD_GETUIVAL1,          // answer with values of I12 and V3.3
    CMD_REINIT_SENSORS,     // (re)init sensors
    CMD_GETBUILDNO,         // request for firmware build number
    CMD_SYSTIME,            // get system time
    CMD_RESET_MCU,          // reset MCU
    // dummy commands for test purposes
    CMD_DUMMY0 = 0xDA,
    CMD_DUMMY1 = 0xAD
} CAN_commands;

void can_messages_proc();
CAN_status can_send_cmd(uint16_t targetID, uint8_t cmd);
CAN_status can_send_data(uint8_t *data, uint8_t len);
int8_t send_temperatures(int8_t N);
