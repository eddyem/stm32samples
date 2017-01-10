/*
 * ccdcacm.h
 *
 * Copyright 2014 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 */

#pragma once
#ifndef __CCDCACM_H__
#define __CCDCACM_H__

#include <libopencm3/usb/usbd.h>

// commands through EP0
#define SEND_ENCAPSULATED_COMMAND   0x00
#define GET_ENCAPSULATED_RESPONSE   0x01
#define SET_COMM_FEATURE            0x02
#define GET_COMM_FEATURE            0x03
#define CLEAR_COMM_FEATURE          0x04
#define SET_LINE_CODING             0x20
#define GET_LINE_CODING             0x21
#define SET_CONTROL_LINE_STATE      0x22
#define SEND_BREAK                  0x23

// Size of input/output buffers
#define USB_TX_DATA_SIZE            64
#define USB_RX_DATA_SIZE            64

// USB connection flag
extern uint8_t USB_connected;
extern struct usb_cdc_line_coding linecoding;

extern char usbdatabuf[];
extern int usbdatalen;

usbd_device *USB_init();
void usb_send(uint8_t byte);
void usb_send_buffer();

#endif // __CCDCACM_H__
