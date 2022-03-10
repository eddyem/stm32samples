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

// LED0 - PC13 (bluepill), blinking each second
#define LED0_port   GPIOC
#define LED0_pin    (1<<13)

// USB pullup (not present in bluepill) - PA15
#define USBPU_port  GPIOA
#define USBPU_pin   (1<<15)
#define USBPU_ON()  pin_clear(USBPU_port, USBPU_pin)
#define USBPU_OFF() pin_set(USBPU_port, USBPU_pin)

#define LED_blink(x)    pin_toggle(x ## _port, x ## _pin)
#define LED_on(x)       pin_clear(x ## _port, x ## _pin)
#define LED_off(x)      pin_set(x ## _port, x ## _pin)

// SPI DMA channel
#define DMA_SPI_Channel DMA1_Channel3
// SCREEN PINs: A,B - PB6,PB7; SCLK - PA6; nOE - PA13
#define A_port      GPIOB
#define A_pin       (1<<6)
#define B_port      GPIOB
#define B_pin       (1<<7)
#define SCLK_port   GPIOA
#define SCLK_pin    (1<<6)
#define nOE_port    GPIOA
//#define nOE_pin     (1<<13)
#define nOE_pin     (1<<4)
#define SET(x)      pin_set(x ## _port, x ## _pin)
#define CLEAR(x)    pin_clear(x ## _port, x ## _pin)
#define TOGGLE(x)   pin_toggle(x ## _port, x ## _pin)

void hw_setup();

#endif // __HARDWARE_H__
