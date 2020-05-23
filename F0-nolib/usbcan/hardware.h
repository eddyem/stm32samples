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

#include <stm32f0.h>

#define CONCAT(a,b)     a ## b
#define STR_HELPER(s)   #s
#define STR(s)          STR_HELPER(s)

#define FORMUSART(X)    CONCAT(USART, X)
#define USARTX          FORMUSART(USARTNUM)

// LEDS: 0 - PC13, 1 - PC14
// LED0
#define LED0_port   GPIOC
#define LED0_pin    (1<<13)
// LED1
#define LED1_port   GPIOC
#define LED1_pin    (1<<14)

#define LED_blink(x)    pin_toggle(x ## _port, x ## _pin)
#define LED_on(x)       pin_clear(x ## _port, x ## _pin)
#define LED_off(x)      pin_set(x ## _port, x ## _pin)


// CAN address - PB14(0), PB15(1), PA8(2)
#define READ_CAN_INV_ADDR()  (((GPIOA->IDR & (1<<8))>>6)|((GPIOB->IDR & (3<<14))>>14))


extern volatile uint32_t Tms;

void gpio_setup(void);
void iwdg_setup();
void pause_ms(uint32_t pause);

#endif // __HARDWARE_H__
