/*
 * This file is part of the canbus4bta project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "hardware.h"

// input buffers size
#define UARTBUFSZ  (80)

void usart_rstbuf();
void usart_setup();
void usart_deinit();
int usart_getline(char **line);
void usart_send(const char *str);
void usart_sendn(const char *str, int L);
void usart_putchar(const char ch);
