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

#include <stm32f3.h>

#define USBPU_port  GPIOA
#define USBPU_pin   (1<<10)
#define USBPU_ON()  pin_clear(USBPU_port, USBPU_pin)
#define USBPU_OFF() pin_set(USBPU_port, USBPU_pin)

#define CFG_port    GPIOA
#define CFG_pin     (1<<9)
#define CFG_ON()    (CFG_port->IDR & CFG_pin)

// RS-485 Rx is low level, Tx - high
#define RX485(port, pin)    do{port->BRR  = pin;}while(0)
#define TX485(port, pin)    do{port->BSRR = pin;}while(0)

extern volatile uint32_t Tms;
extern uint8_t Config_mode;

void hw_setup();

