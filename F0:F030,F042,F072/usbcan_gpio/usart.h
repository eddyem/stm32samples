/*
 * This file is part of the usbcangpio project.
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

#include <stdint.h>

// DMA linear buffers for Rx/Tx
#define DMARXBUFSZ          128
#define DMATXBUFSZ          128
// incoming ring buffer - only if there's a lot of data in DMA RX buffer
#define RXRBSZ              256

#define USART_MIN_SPEED     1024
#define USART_MAX_SPEED     1000000
#define USART_DEFAULT_SPEED  9600

typedef struct{
    uint32_t speed;         // baudrate
    uint8_t idx : 1;        // Usart idx (0/1 for USART1/USART2)
    uint8_t RXen : 1;       // enable rx
    uint8_t TXen : 1;       // enable tx
    uint8_t textproto : 1;  // match '\n' and force output by lines (if there's enough place in buffers and monitor == 1)
    uint8_t monitor : 1;    // async output by incoming (over '\n', IDLE or buffer full)
} usartconf_t;

int usart_config(usartconf_t *config);
int chkusartconf(usartconf_t *c);

int usart_start();
void usart_stop();

int get_curusartconf(usartconf_t *c);
void get_defusartconf(usartconf_t *c);

int usart_process(uint8_t *buf, int len);

int usart_receive(uint8_t *buf, int len);
int usart_send(const uint8_t *data, int len);
