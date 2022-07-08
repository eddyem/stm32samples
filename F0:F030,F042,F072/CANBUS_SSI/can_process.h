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
#include "flash.h"

// timeout for trying to send data
#define SEND_TIMEOUT_MS     (10)

// 8-bit commands sent by master
typedef enum{
    CMD_PING,               // just echo it back
    CMD_GETMCUTEMP,         // MCU temperature value
    CMD_GETUVAL,            // answer with values of V12 and V5
    CMD_GETU3V3,          // answer with values of V3.3
} CAN_commands;

void can_messages_proc();
CAN_status try2send(uint8_t *buf, uint8_t len, uint16_t id);
