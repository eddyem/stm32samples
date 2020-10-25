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

// Buzzer: PA14
#define BUZZER_port     GPIOA
#define BUZZER_pin      (1<<14)
// Cooler0/1 power on/off: PB4, PB5
#define COOLER0_port    GPIOB
#define COOLER0_pin     (1<<4)
#define COOLER1_port    GPIOB
#define COOLER1_pin     (1<<5)
// Relay: PC13
#define RELAY_port      GPIOC
#define RELAY_pin       (1<<13)
// Buttons: PB14/15
#define BUTTON0_port    GPIOB
#define BUTTON0_pin     (1<<14)
#define BUTTON1_port    GPIOB
#define BUTTON1_pin     (1<<15)
// common macro for getting status
#define CHK(x)          ((x ## _port->IDR & (x ## _pin)) ? 1 : 0)

#define ON(x)           do{pin_set(x ## _port, x ## _pin);}while(0)
#define OFF(x)          do{pin_clear(x ## _port, x ## _pin);}while(0)

extern volatile uint32_t Tms;
extern volatile uint32_t Cooler0speed;
extern volatile uint32_t Cooler1speed;
extern volatile uint32_t Cooler1RPM;
extern uint8_t ledsON;

void HW_setup(void);
void adc_setup();
void iwdg_setup();
void pause_ms(uint32_t pause);
void Jump2Boot();

#endif // __HARDWARE_H__
