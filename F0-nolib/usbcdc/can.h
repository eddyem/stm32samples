/*
 *                                                                                                  geany_encoding=koi8-r
 * can.h
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
#ifndef __CAN_H__
#define __CAN_H__

#include "hardware.h"

// amount of filter banks in STM32F0
#define STM32F0FBANKNO      28
// flood period in milliseconds
#define FLOOD_PERIOD_MS     5

// simple 1-byte commands
#define CMD_TOGGLE      (0xDA)
#define CMD_BCAST       (0xAD)
// mask heading 8 bits of can ID
#define CAN_ID_MASK     (0x7F8)
// prefix to make ID from any number (0..7)
#define CAN_ID_PREFIX   (0xAAA)
// "target" ID: num=0
#define TARG_ID         (CAN_ID_PREFIX & CAN_ID_MASK)
// "broadcast" ID: all ones
#define BCAST_ID        (0x7FF)

// incoming message buffer size
#define CAN_INMESSAGE_SIZE  (8)

// CAN message
typedef struct{
    uint8_t data[8];    // up to 8 bytes of data
    uint8_t length;     // data length
    //uint8_t filterNo;   // filter number
    //uint8_t fifoNum;    // message FIFO number
    uint16_t ID;        // ID of receiver
} CAN_message;

typedef enum{
    CAN_STOP,
    CAN_READY,
    CAN_BUSY,
    CAN_OK,
    CAN_FIFO_OVERRUN
} CAN_status;

CAN_status CAN_get_status();

void readCANID();
uint16_t getCANID();

void CAN_reinit(uint16_t speed);
void CAN_setup(uint16_t speed);

CAN_status can_send(uint8_t *msg, uint8_t len, uint16_t target_id);
void can_send_dummy();
void can_send_broadcast();
void can_proc();

CAN_message *CAN_messagebuf_pop();

void set_flood(CAN_message *msg);

#endif // __CAN_H__
