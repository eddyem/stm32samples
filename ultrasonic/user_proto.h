/*
 * user_proto.h
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
#ifndef __USER_PROTO_H__
#define __USER_PROTO_H__

#include "cdcacm.h"

// shorthand for prnt
#define P(arg)   do{prnt((uint8_t*)arg);}while(0)
// debug message - over USB
#ifdef EBUG
	#define DBG(a)    do{prnt((uint8_t*)a);}while(0)
#else
	#define DBG(a)
#endif

extern uint8_t cont;

typedef uint8_t (*intfun)(int32_t);

void prnt(uint8_t *wrd);
#define newline()   usb_send('\n')

void print_int(int32_t N);
void print_hex(uint8_t *buff, uint8_t l);

int parce_incoming_buf(char *buf, int len);

#endif // __USER_PROTO_H__
