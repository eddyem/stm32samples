/*
 *                                                                                                  geany_encoding=koi8-r
 * hardware.h
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 *
 */
#pragma once
#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include "stm32f1.h"

// LEDS: 0 - PA0, 1 - PA4
// LED0 - blinking each second
#define LED0_port   GPIOA
#define LED0_pin    (1<<0)
// LED1 - PWM
#define LED1_port   GPIOA
#define LED1_pin    (1<<4)

// Buttons' state: PA14 (1)/PA15 (0)
#define GET_BTN0()  ((GPIOA->IDR & (1<<15)) ? 0 : 1)
#define GET_BTN1()  ((GPIOA->IDR & (1<<14)) ? 0 : 1)

// USB pullup (not used in STM32F0x2!) - PA13
#define USBPU_port  GPIOA
#define USBPU_pin   (1<<13)
#define USBPU_ON()  pin_clear(USBPU_port, USBPU_pin)
#define USBPU_OFF() pin_set(USBPU_port, USBPU_pin)

#define LED_blink(x)    pin_toggle(x ## _port, x ## _pin)
#define LED_on(x)       pin_clear(x ## _port, x ## _pin)
#define LED_off(x)      pin_set(x ## _port, x ## _pin)

void hw_setup();

#endif // __HARDWARE_H__
