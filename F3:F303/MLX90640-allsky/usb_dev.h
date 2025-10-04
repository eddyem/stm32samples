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

#include <stm32f3.h>
#include "usb_lib.h"

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

extern usb_LineCoding lineCoding;
extern volatile uint8_t CDCready;

void break_handler();
void clstate_handler(uint16_t val);
void linecoding_handler(usb_LineCoding *lc);


// sizes of ringbuffers for outgoing and incoming data
#define RBOUTSZ     (1024)
#define RBINSZ      (128)

#define newline()   USB_putbyte('\n')
#define UN(s)     do{USB_sendstr(s); USB_putbyte('\n');}while(0)
#define U(s)        USB_sendstr(s)

#ifdef EBUG
#define DBG(s)      do{USB_sendstr(s); USB_putbyte('\n');}while(0)
#else
#define DBG(s)
#endif

int USB_sendall();
int USB_send(const uint8_t *buf, int len);
int USB_putbyte(uint8_t byte);
int USB_sendstr(const char *string);
int USB_receive(uint8_t *buf, int len);
int USB_receivestr(char *buf, int len);
