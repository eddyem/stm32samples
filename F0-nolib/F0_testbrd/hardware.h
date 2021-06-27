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

#include "stm32f0.h"

#define CONCAT(a,b)     a ## b
#define STR_HELPER(s)   #s
#define STR(s)          STR_HELPER(s)

// PWM LEDS
#define SET_LED_PWM3(ch, N) do{TIM3->CCR ## ch = (uint32_t)N;}while(0)
#define GET_LED_PWM3(ch)    (uint8_t)(TIM3->CCR ## ch)
#define SET_LED_PWM1(N)     do{TIM1->CCR1 = (uint32_t)N;}while(0)
#define GET_LED_PWM1()      (uint8_t)(TIM1->CCR1)

// USB pullup (not used in STM32F0x2!) - PA15
#define USBPU_port  GPIOA
#define USBPU_pin   (1<<15)

void iwdg_setup();
void hw_setup();

#endif // __HARDWARE_H__
