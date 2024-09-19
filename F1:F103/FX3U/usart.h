/*
 * This file is part of the fx3u project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#define UARTBUFSZI  (64)
#define UARTBUFSZO  (128)

#define usartrx()  (usart_linerdy)
#define usartovr() (usart_bufovr)

int usart_transmit();
void usart_setup(uint32_t speed);
int usart_getline(char **line);
int usart_send(const char *str);
int usart_putchar(const char ch);
