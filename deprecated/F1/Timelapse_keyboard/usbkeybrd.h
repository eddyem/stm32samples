/*
 * usbkeybrd.h
 *
 * Copyright 2015 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#ifndef __USBKEYBRD_H__
#define __USBKEYBRD_H__

#include "main.h"

extern usbd_device *usbd_dev;

void process_usbkbrd();
void send_msg(char *msg);
void put_char_to_buf(char ch);
#define P(x) send_msg(x)
void usbkeybrd_setup();

void print_hex(uint8_t *buff, uint8_t l);
void print_int(int32_t N);

//void newline();
#define newline()  do{put_char_to_buf('\n');}while(0)

#define poll_usbkeybrd() usbd_poll(usbd_dev)

#endif // __USBKEYBRD_H__
