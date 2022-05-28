/*
 * usart.h
 *
 * Copyright 2018 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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

// input and output buffers size
#define UARTBUFSZI  (16)
#define UARTBUFSZO  (32)

// macro for static strings
#define USEND(str) usart_send(str)

#ifdef EBUG
#define x__(a) #a
#define STR(a) x__(a)
#define DBG(str)  do{usart_send(__FILE__ " (L" STR(__LINE__) "): " str); \
    usart_putchar('\n'); usart_transmit(); }while(0)
#else
#define DBG(str)
#endif


#define usartrx()  (usart_linerdy)
#define usartovr() (usart_bufovr)

extern volatile int usart_linerdy, usart_bufovr, usart_txrdy;

void usart_transmit();
void usart_setup();
int usart_getline(char **line);
void usart_send(const char *str);
void usart_putchar(const char ch);

#endif // __USART_H__
