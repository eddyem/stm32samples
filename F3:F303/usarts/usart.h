/*
 * This file is part of the F303usart project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#ifndef __USART_H__
#define __USART_H__

#include "hardware.h"

#define USARTNUM    (3)

// input and output buffers size
#define UARTBUFSZI  (80)
#define UARTBUFSZO  (80)
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
void usart_stop();

#endif // __USART_H__
