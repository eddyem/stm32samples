/*
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
#include "usb_descr.h"
#include "usb_lib.h"

// interfaces:
#define I_CMD   (0)
#define I_X     (1)
#define I_Y     (2)

typedef struct {
    uint32_t dwDTERate;
    uint8_t bCharFormat;
#define USB_CDC_1_STOP_BITS   0
#define USB_CDC_1_5_STOP_BITS 1
#define USB_CDC_2_STOP_BITS   2
    uint8_t bParityType;
#define USB_CDC_NO_PARITY     0
#define USB_CDC_ODD_PARITY    1
#define USB_CDC_EVEN_PARITY   2
#define USB_CDC_MARK_PARITY   3
#define USB_CDC_SPACE_PARITY  4
    uint8_t bDataBits;
} __attribute__ ((packed)) usb_LineCoding;

extern volatile uint8_t CDCready[bTotNumEndpoints];

void break_handler(uint8_t ifno);
void clstate_handler(uint8_t ifno, uint16_t val);
void linecoding_handler(uint8_t ifno, usb_LineCoding *lc);

// as ugly CDC have no BREAK after disconnected client in non-canonical mode, we should use timeout - more than 2ms
#define DISCONN_TMOUT   (1000)

// sizes of ringbuffers for outgoing and incoming data
#define RBOUTSZ     (512)
#define RBINSZ      (128)

#define newline(ifno)   USB_putbyte(ifno, '\n')
#define USND(ifno, s)   do{USB_sendstr(ifno, s); USB_putbyte(ifno, '\n');}while(0)
#define CMDWR(s)        USB_sendstr(I_CMD, s)
#define CMDWRn(s)       do{USB_sendstr(I_CMD, s); USB_putbyte(I_CMD, '\n');}while(0)
#define CMDn()          USB_putbyte(I_CMD, '\n')

#ifdef EBUG
#define STR_HELPER(s)   #s
#define STR(s)          STR_HELPER(s)
#define DBG(str)  do{USB_sendstr(I_CMD, __FILE__ " (L" STR(__LINE__) "): " str); USB_putbyte(I_CMD, '\n');}while(0)
#define DBGs(str) do{USB_sendstr(I_CMD, str); USB_sendstr(I_CMD, "\n");}while(0)
#else
#define DBG(s)
#define DBGs(s)
#endif

int USB_sendall(uint8_t ifno);
int USB_send(uint8_t ifno, const uint8_t *buf, int len);
int USB_putbyte(uint8_t ifno, uint8_t byte);
int USB_sendstr(uint8_t ifno, const char *string);
int USB_receive(uint8_t ifno, uint8_t *buf, int len);
int USB_receivestr(uint8_t ifno, char *buf, int len);
