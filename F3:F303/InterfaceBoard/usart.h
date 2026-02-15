/*
 * This file is part of the multiiface project.
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

#include "hardware.h"
#include "usb_dev.h"

// DMA linear buffers for Rx/Tx
#define DMARXBUFSZ      128
#define DMATXBUFSZ      128
// ringbuffers for collected data
#define USARTRXRBSZ     256
#define USARTTXRBSZ     256

void usart_config(uint8_t ifNo, usb_LineCoding *lc);
void usart_start(uint8_t ifNo);
void usart_stop(uint8_t ifNo);

void usarts_process();

int usart_send(uint8_t ifNo, const uint8_t *data, int len);
//int usart_receive(uint8_t ifNo, uint8_t *data, int len);
