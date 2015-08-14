/*
 * uart.h
 *
 * Copyright 2014 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#ifndef __UART_H__
#define __UART_H__

// Size of buffers
#define UART_BUF_DATA_SIZE            128

typedef struct {
	uint8_t buf[UART_BUF_DATA_SIZE];
	uint8_t start; // index from where to start reading
	uint8_t end;   // index from where to start writing
} UART_buff;

void UART_init(uint32_t UART);
void UART_setspeed(uint32_t UART);

void fill_uart_buff(uint32_t UART, uint8_t byte);
void uart1_send(uint8_t byte);
void uart2_send(uint8_t byte);

uint8_t *check_UART2();

UART_buff *get_uart_buffer(uint32_t UART);

#endif // __UART_H__
