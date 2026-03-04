/*
 * This file is part of the test project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#define USARTTXBUFSZ    (4096)
#define USARTRXBUFSZ    (128)
#define USARTTXDMABUFSZ (1024)

// blocking timeout - not more than 5ms
#define USARTBLKTMOUT   (5)
// send buffer each 10ms
#define USARTSENDTMOUT  (10)

typedef union{
    struct{
        uint8_t txerr   : 1; // transmit error
        uint8_t rxovrfl : 1; // receive buffer overflow
    };
    uint8_t all;
} USART_flags_t;

void usart_setup(uint32_t speed);
int usart_send(const char *str, int len);
char *usart_getline();
int usart_sendstr(const char *str);
USART_flags_t usart_process();
