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

#define SYSMEM03x 0x1FFFEC00
#define SYSMEM04x 0x1FFFC400
#define SYSMEM05x 0x1FFFEC00
#define SYSMEM07x 0x1FFFC800
#define SYSMEM09x 0x1FFFD800

#define SystemMem SYSMEM07x


#define CONCAT(a,b)     a ## b
#define STR_HELPER(s)   #s
#define STR(s)          STR_HELPER(s)

// LEDS: 0 - PB0, 1 - PB1
// LED0
#define LED0_port   GPIOB
#define LED0_pin    (1<<0)
// LED1
#define LED1_port   GPIOB
#define LED1_pin    (1<<1)

#define LED_blink(x)    do{if(ledsON) pin_toggle(x ## _port, x ## _pin);}while(0)
#define LED_on(x)       do{if(ledsON) pin_clear(x ## _port, x ## _pin);}while(0)
#define LED_off(x)      do{pin_set(x ## _port, x ## _pin);}while(0)

extern volatile uint32_t Tms;

extern uint8_t ledsON;

void gpio_setup(void);
void iwdg_setup();
void pause_ms(uint32_t pause);
void Jump2Boot();

#endif // __HARDWARE_H__
