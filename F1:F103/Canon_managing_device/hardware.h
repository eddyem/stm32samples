/*
 * This file is part of the canonmanage project.
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

#include <stm32f1.h>

// USB DP pullup: PA10
#define USBPU_port  GPIOA
#define USBPU_pin   (1<<10)
#define USBPU_ON()  pin_clear(USBPU_port, USBPU_pin)
#define USBPU_OFF() pin_set(USBPU_port, USBPU_pin)

// Ven (enable lens voltage) - PA8
#define VEN_port    GPIOA
#define VEN_pin     (1<<8)
#define LENS_ON()   pin_set(VEN_port, VEN_pin)
#define LENS_OFF()  pin_clear(VEN_port, VEN_pin)

// CAN-USB switch: PC13, pullup
#define USBCAN_port GPIOC
#define USBCAN_pin  (1<<13)
#define ISUSB()     (USBCAN_port->IDR & USBCAN_pin)

// Lens detect: PA4, pullup
#define LENSDET_port    GPIOA
#define LENSDET_pin     (1<<4)
#define LENSCONNECTED() (0 == (LENSDET_port->IDR & LENSDET_pin))

// Overcurrent detect: PA9, pullup
#define OC_port         GPIOA
#define OC_pin          (1<<9)
#define OVERCURRENT()   (0 == (OC_port->IDR & OC_pin))

extern volatile uint32_t Tms;

void hw_setup();

#define SPIx            SPI1
#define SPI_APB2        RCC_APB2ENR_SPI1EN
