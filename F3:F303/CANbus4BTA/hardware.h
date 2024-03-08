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

#include <stm32f3.h>

// PEP emulation - period of encoder/esw data send - 70ms
#define ENCODER_PERIOD  (69)

// USB pullup
#define USBPU_port  GPIOC
#define USBPU_pin   (1<<15)
#define USBPU_ON()  pin_set(USBPU_port, USBPU_pin)
#define USBPU_OFF() pin_clear(USBPU_port, USBPU_pin)

// relay
#define RELAY_port  GPIOC
#define RELAY_pin   (1<<14)
#define RELAY_ON()  pin_set(RELAY_port, RELAY_pin)
#define RELAY_OFF() pin_clear(RELAY_port, RELAY_pin)
#define RELAY_GET() pin_read(RELAY_port, RELAY_pin)

// GPIO end-switches multiplexer (addr - PB0..2, ~DEN0 - PA5, ~DEN1 - PA6) and input (PA4)
#define ESW_GET()   (GPIOA->IDR & (1<<4) ? 0 : 1)
#define MULADDR_set(x)  do{GPIOB->BSRR = (x&7) | (((~x)&7) << 16);}while(0)
#define MUL_ON(x)   pin_clear(GPIOA, 1<<(5+x))
#define MUL_OFF(x)  pin_set(GPIOA, 1<<(5+x))

extern volatile uint32_t Tms;

// SPI1 is encoder, SPI2 is ext
#define ENCODER_SPI     (1)

void hw_setup();
