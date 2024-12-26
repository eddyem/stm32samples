/*
 * This file is part of the hallinear project.
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


// CAN bus oscillator frequency: 36MHz
#define CAN_F_OSC       (36000000UL)
// timing values TBS1 and TBS2 (in BTR [TBS1-1] and [TBS2-1])
// use 3 and 2 to get 6MHz
#define CAN_TBS1    (3)
#define CAN_TBS2    (2)
// bitrate oscillator frequency
#define CAN_BIT_OSC (CAN_F_OSC / (1+CAN_TBS1+CAN_TBS2))

#define CAN_MIN_SPEED   (9600)
#define CAN_MAX_SPEED   (3000000)

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

void CAN_setup();
CAN_status CAN_send(CAN_message *msg);
void CAN_proc();
CAN_message *CAN_receive();
void CAN_reinit();
