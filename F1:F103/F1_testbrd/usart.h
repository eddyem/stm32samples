/*
 * This file is part of the F1_testbrd project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

