/*
 * This file is part of the F303usartDMA project.
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

// max pause (ms) from buffer filling and data transmitting
#define TRANSMIT_DELAY  (15)

// input and output buffers size (don't forget trailing zero in input buffer!)
// DMA generates TC irq while last byte isn't in buffer, so, to get 80 symbols excluding '\n' we need 82 bytes in buffer
#define UARTBUFSZI  (82)
#define UARTBUFSZO  (80)

// timeout between data bytes
#ifndef TIMEOUT_MS
#define TIMEOUT_MS (1500)
#endif

// macro for static strings
#define SEND(str) usart_send(str)

#define newline()   usart_putchar('\n')

int usart_ovr();
int chk_usart();
void usart_setup();
int usart_getline(char **line);
void usart_send(const char *str);
void usart_sendn(const char *str, uint32_t L);
void usart_putchar(const char ch);
void usart_stop();

#endif // __USART_H__
