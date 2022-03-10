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

#include <stdint.h>

// input and output buffers size
#define UARTBUFSZI  (32)
#define UARTBUFSZO  (512)
// timeout between data bytes
#ifndef TIMEOUT_MS
#define TIMEOUT_MS (1500)
#endif

// macro for static strings
#define SEND(str) usart_send(str)

#define STR_HELPER(s)   #s
#define STR(s)          STR_HELPER(s)

#ifdef EBUG
#define MSG(str)  do{SEND(__FILE__ " (L" STR(__LINE__) "): " str);}while(0)
#define DBG(str)  do{SEND(str); usart_putchar('\n'); }while(0)
#else
#define MSG(str)
#define DBG(str)
#endif

#define usartrx()  (linerdy)
#define usartovr() (bufovr)

extern volatile int linerdy, bufovr, txrdy;

void transmit_tbuf();
void usart_setup();
int usart_getline(char **line);
void usart_send(const char *str);
void newline();
void usart_putchar(const char ch);
char *u2str(uint32_t val);
void printu(uint32_t val);
void printuhex(uint32_t val);
void hexdump(uint8_t *arr, uint16_t len);
void hexdump16(uint16_t *arr, uint16_t len);
void hexdump32(uint32_t *arr, uint16_t len);

#endif // __USART_H__
