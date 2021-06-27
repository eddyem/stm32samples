/*
 * usart.h
 *
 * Copyright 2017 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#ifndef __USART_H__
#define __USART_H__

#include "hardware.h"

#ifdef USART3
#define USARTNUM    (3)
#else
#define USARTNUM    (2)
#endif


// input and output buffers size
#define UARTBUFSZI  (32)
#define UARTBUFSZO  (512)
// timeout between data bytes
#ifndef TIMEOUT_MS
#define TIMEOUT_MS (1500)
#endif

// macro for static strings
#define SEND(n, str) usart_send(n, str)

#define usartrx(n)  (linerdy[n-1])
#define usartovr(n) (bufovr[n-1])

extern volatile int linerdy[], bufovr[], txrdy[];

void transmit_tbuf();
void usart_setup();
int usart_getline(int usartno, char **line);
void usart_send(int usartno, const char *str);
void usart_sendn(int usartno, const char *str, uint32_t L);
void newline(int usartno);
void usart_putchar(int usartno, const char ch);

#endif // __USART_H__
