/*
 * This file is part of the canuart project.
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

// input and output buffers size
#define UARTBUFSZI  (256)
#define UARTBUFSZO  (1024)

#define usartrx()  (usart_linerdy)
#define usartovr() (usart_bufovr)

extern volatile int usart_txrdy;

int usart_transmit();
void usart_setup();
int usart_getline(char **line);
int usart_send(const char *str);
int usart_putchar(const char ch);
