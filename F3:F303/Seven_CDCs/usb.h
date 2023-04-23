/*
 * This file is part of the SevenCDCs project.
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

#include "ringbuffer.h"
#include "usbhw.h"

// sizes of ringbuffers for outgoing and incoming data
#define RBOUTSZ     (128)
#define RBINSZ      (128)

#define newline(x)  USB_putbyte(x, '\n')
#define USND(x, s)  do{USB_sendstr(x, s); USB_putbyte(x, '\n');}while(0)

#define STR_HELPER(s)   #s
#define STR(s)          STR_HELPER(s)

// total amount of working EPs
#define WORK_EPs    7
// functional EPs
#define CMD_EPNO    1
#define USART1_EPNO 2
#define USART2_EPNO 3
#define USART3_EPNO 4
#define CAN_EPNO    6
#define DBG_EPNO    7
#define USARTMAX_EPNO USART3_EPNO
// functional indexes
#define CMD_IDX     (CMD_EPNO-1)
#define CAN_IDX     (CAN_EPNO-1)
#define DBG_IDX     (DBG_EPNO-1)

extern volatile ringbuffer rbout[], rbin[];
extern volatile uint8_t bufisempty[], bufovrfl[];

void send_next(int ifNo);
int USB_sendall(int ifNo);
int USB_send(int ifNo, const uint8_t *buf, int len);
int USB_putbyte(int ifNo, uint8_t byte);
int USB_sendstr(int ifNo, const char *string);
int USB_receive(int ifNo, uint8_t *buf, int len);
int USB_receivestr(int ifNo, char *buf, int len);
