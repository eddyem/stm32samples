/*
 * This file is part of the ir-allsky project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

// timeout of TXrdy waiting
#define RXRDY_TMOUT     (100)
// DMA transmission and reception buffers' size
#define UARTBUFSZI  (128)
#define UARTBUFSZO  (256)
// and circular buffer for big transmissions - 5k
#define DMARBSZ     (5120)

int usart_ovr(); // RX overfull occured
void usart_process(); // send next data portion
int usart_setup(uint32_t speed); // set USART1 with given speed
char *usart_getline(int *len); // read from rbin to buf
int usart_send(const uint8_t *data, int len);
int usart_sendstr(const char *str);
int usart_putbyte(uint8_t ch);
void usart_stop();
