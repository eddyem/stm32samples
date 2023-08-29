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

// identifier mask
#define CAN_ID_MASK     ((uint16_t)0x7F0)
// prefix of identifiers
#define CAN_ID_PREFIX   ((uint16_t)0xAAA)
// this is master - Controller_address==0
#define MASTER_ID       (CAN_ID_PREFIX & CAN_ID_MASK)
// broadcasting to every slave
#define BCAST_ID        ((uint16_t)0x7F7)
// send dummy message to this ID for testing CAN bus status
#define NOONE_ID        ((uint16_t)0x7FF)

extern uint16_t curcanspeed;
extern uint16_t CANID;
extern int8_t cansniffer;

typedef struct{
    uint8_t data[8];
    uint8_t length;
    uint16_t ID;        // ID of receiver
} CAN_message;

typedef enum{
    CAN_NOTMASTER,      // can't send command - not a master
    CAN_STOP,           // CAN stopped
    CAN_READY,          // ready to send
    CAN_BUSY,           // bus is busy
    CAN_OK,             // all OK?
    CAN_FIFO_OVERRUN,   // FIFO overrun
    CAN_ERROR           // no recipients on bus or too many errors
} CAN_status;

CAN_status CAN_get_status();

void readCANID();

void CAN_setup(uint16_t speed);
void CAN_listenall();
void CAN_listenone();

void can_proc();
CAN_status can_send(uint8_t *msg, uint8_t len, uint16_t target_id);

CAN_message *CAN_messagebuf_pop();

#endif // __CAN_H__
