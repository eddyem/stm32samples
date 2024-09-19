/*
 * This file is part of the fx3u project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32f1.h>
#include "can.h"

// command parameter flag means this is a setter
#define SETTER_FLAG     (0x80)
#define ISSETTER(data)  ((data[2] & SETTER_FLAG))
// parameter number 127 means there no parameter number at all (don't need paremeter or get all)
#define NO_PARNO        (0x7f)
// base value of parameter (even if it is a setter)
#define PARBASE(x)      (x & 0x7f)
// get parameter value of msg->data
#define PARVAL(data)    (data[2] & 0x7f)

// make error for CAN answer
#define FORMERR(m, err) do{m->data[3] = err; if(m->length < 4) m->length = 4;}while(0)

// error codes for answer message
typedef enum{
    ERR_OK,         // 0 - all OK
    ERR_BADPAR,     // 1 - parameter is wrong
    ERR_BADVAL,     // 2 - wrong value
    ERR_WRONGLEN,   // 3 - wrong message length
    ERR_BADCMD,     // 4 - unknown command
    ERR_CANTRUN,    // 5 - can't run given command due to bad parameters or other
    ERR_AMOUNT      // amount of error codes
} errcodes;

// set command bytes in CAN message
#define MSG_SET_CMD(msg, cmd)       do{*((uint16_t*)msg.data) = (cmd);}while(0)
#define MSGP_SET_CMD(msg, cmd)      do{*((uint16_t*)msg->data) = (cmd);}while(0)
// set error
#define MSG_SET_ERR(msg, err)       do{msg.data[3] = (err);}while(0)
#define MSGP_SET_ERR(msg, err)      do{msg->data[3] = (err);}while(0)
// set uint32_t data
#define MSG_SET_U32(msg, d)         do{*((uint32_t*)(&msg.data[4])) = (d);}while(0)
#define MSGP_SET_U32(msg, d)        do{*((uint32_t*)(&msg->data[4])) = (d);}while(0)
// get uint32_t data
#define MSG_GET_U32(msg)            (*(uint32_t*)&msg.data[4])
#define MSGP_GET_U32(msg)           (*(uint32_t*)&msg->data[4])

// CAN commands indexes
enum{
    CMD_PING,       // just ping
    CMD_RESET,      // reset MCU
    CMD_TIME,       // get/set Tms
    CMD_MCUTEMP,    // get MCU temperature (*10)
    CMD_ADCRAW,     // get ADC raw values
    CMD_CANSPEED,   // get/set CAN speed (kbps)
    CMD_CANID,      // get/set common CAN ID (both in and out)
    CMD_CANIDin,    // input CAN ID
    CMD_CANIDout,   // output CAN ID
    CMD_SAVECONF,   // save configuration
    CMD_ERASESTOR,  // erase all flash storage
    CMD_RELAY,      // switch relay ON/OFF
    CMD_GETESW,     // current ESW state, bounce-free
    CMD_GETESWNOW,  // current ESW state, absolute
    CMD_BOUNCE,     // get/set bounce constant (ms)
    CMD_USARTSPEED, // get/set USART1 speed (if encoder on RS-422)
    CMD_LED,        // onboard LED
    CMD_FLAGS,      // flags setter/getter
    CMD_INCHNLS,    // all bits set are active supported IN channels
    CMD_OUTCHNLS,   // all bits set are active supported OUT channels
    CMD_MODBUSID,   // set/get modbus slave ID (or 0 if master)
    CMD_MODBUSSPEED,// speed of modbus interface
    // should be the last:
    CMD_AMOUNT     // amount of CAN commands
};

void run_can_cmd(CAN_message *msg);
