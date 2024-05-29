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

#include <stm32f1.h>
#include "hardware.h"

#define BUFSZ   (64)

typedef struct{
    uint32_t can_monitor : 1;
} flags_t;

extern flags_t flags;

#ifdef EBUG
#define DBG(str)  do{usart_send(__FILE__ " (L" STR(__LINE__) "): " str); \
    usart_putchar('\n'); usart_transmit(); }while(0)
#else
#define DBG(str)
#endif


void cmd_parser(char *buf);
