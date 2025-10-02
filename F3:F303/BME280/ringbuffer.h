/*
 * This file is part of the bme280 project.
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

/*
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

#if defined STM32F0
#include <stm32f0.h>
#elif defined STM32F1
#include <stm32f1.h>
#elif defined STM32F3
#include <stm32f3.h>
#endif

typedef struct{
    uint8_t *data;      // data buffer
    const int length;   // its length
    int head;           // head index
    int tail;           // tail index
    volatile int busy; // == TRUE if buffer is busy now
} ringbuffer;

int RB_read(ringbuffer *b, uint8_t *s, int len);
int RB_readto(ringbuffer *b, uint8_t byte, uint8_t *s, int len);
int RB_hasbyte(ringbuffer *b, uint8_t byte);
int RB_write(ringbuffer *b, const uint8_t *str, int l);
int RB_datalen(ringbuffer *b);
int RB_clearbuf(ringbuffer *b);
