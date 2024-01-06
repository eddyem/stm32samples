/*
 * This file is part of the canbus4bta project.
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

#include <stdint.h>

// amount of filter banks in STM32F0
#define STM32F0FBANKNO      28
// flood period in milliseconds
#define FLOOD_PERIOD_MS     5

// incoming message buffer size
#define CAN_INMESSAGE_SIZE  (8)
extern uint32_t floodT;

// CAN message
typedef struct{
    uint8_t data[8];    // up to 8 bytes of data
    uint8_t length;     // data length
    uint16_t ID;        // ID of receiver
} CAN_message;

typedef enum{
    CAN_STOP,
    CAN_READY,
    CAN_BUSY,
    CAN_OK,
    CAN_ERR,
    CAN_FIFO_OVERRUN
} CAN_status;

CAN_status CAN_get_status();

void CAN_reinit(uint16_t speed);
void CAN_setup(uint32_t speed);

CAN_status CAN_send(CAN_message *message);
void CAN_proc();
void CAN_printerr();

CAN_message *CAN_messagebuf_pop();

void CAN_flood(CAN_message *msg, int incr);
uint32_t CAN_speed();
