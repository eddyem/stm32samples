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

#include <stdint.h>

// input and output buffers size
#define UARTBUFSZI  (64)
#define UARTBUFSZO  (64)
// timeout between data bytes
#ifndef TIMEOUT_MS
#define TIMEOUT_MS (1500)
#endif

// macro for static strings
#define SEND(str) usart_send(str)
#define _s(s)   #s
#define STR(s)  _s(s)

#ifdef EBUG
#define DBG(str)  do{SEND(__FILE__ " (L" STR(__LINE__) "): " str); newline();}while(0)
#define HERE()    do{SEND(STR(__LINE__)); usart_putchar('\n');}while(0)
#define MSG(str)  do{SEND(str); usart_putchar('\n');}while(0)
#else
#define MSG(str)
#define HERE()
#define DBG(str)
#endif

#define usartrx()  (linerdy)
#define usartovr() (bufovr)

extern int linerdy, bufovr, txrdy;

int transmit_tbuf();
void usart_setup();
uint16_t usart_get(uint8_t **line);
void usart_send(const char *str);
void usart_senddata(const uint8_t *str, uint16_t len);
void newline();
void usart_putchar(const char ch);
void printu(uint32_t val);
void printuhex(uint32_t val);
void hexdump(const uint8_t *arr, uint16_t len);

#endif // __USART_H__
