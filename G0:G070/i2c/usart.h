/*
 * This file is part of the usart project.
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

#define UARTBUFSZ   (64)

#define USND(t)  do{usart3_sendstr(t);}while(0)

void usart3_setup();
int usart3_send_blocking(const char *str, int len);
int usart3_send(const char *str, int len);
void usart3_sendbuf();
char *usart3_getline(int *wasbo);
int usart3_sendstr(const char *str);
