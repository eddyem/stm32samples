/*
 * This file is part of the canrelay project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include <stm32f0.h>

// default CAN bus speed in kbaud
#define DEFAULT_CAN_SPEED       (250)

#define SYSMEM03x 0x1FFFEC00
#define SYSMEM04x 0x1FFFC400
#define SYSMEM05x 0x1FFFEC00
#define SYSMEM07x 0x1FFFC800
#define SYSMEM09x 0x1FFFD800
// define SystemMem to other in MAKEFILE
#ifndef SystemMem
#define SystemMem SYSMEM07x
#endif

#define CONCAT(a,b)     a ## b
#define STR_HELPER(s)   #s
#define STR(s)          STR_HELPER(s)

#define FORMUSART(X)    CONCAT(USART, X)
#define USARTX          FORMUSART(USARTNUM)

// LEDs amount
#define LEDSNO      (4)
// LEDs ports & pins
extern GPIO_TypeDef *LEDports[LEDSNO];
extern const uint32_t LEDpins[LEDSNO];
// LEDS: 0 - PB12, 1 - PB13, 2 - PB14, 3 - PB15
#define LED_toggle(x)   do{pin_toggle(LEDports[x], LEDpins[x]);}while(0)
#define LED_on(x)       do{pin_clear(LEDports[x], LEDpins[x]);}while(0)
#define LED_off(x)      do{pin_set(LEDports[x], LEDpins[x]);}while(0)
#define LED_chk(x)      ((LEDports[x]->IDR & LEDpins[x]) ? 0 : 1)

#define RelaysNO    (2)
extern GPIO_TypeDef *R_ports[RelaysNO];
extern const uint32_t R_pins[RelaysNO];
#define Relay_ON(x)     do{pin_set(R_ports[x], R_pins[x]);}while(0)
#define Relay_OFF(x)    do{pin_clear(R_ports[x], R_pins[x]);}while(0)
#define Relay_TGL(x)    do{pin_toggle(R_ports[x], R_pins[x]);}while(0)
#define Relay_chk(x)    (pin_read(R_ports[x], R_pins[x]))

// Buttons amount
#define BTNSNO      (4)
// Buttons ports & pins
extern GPIO_TypeDef *BTNports[BTNSNO];
extern const uint32_t BTNpins[BTNSNO];
// state 1 - pressed, 0 - released
#define BTN_state(x)    ((BTNports[x]->IDR & BTNpins[x]) ? 0 : 1)

// mask for PB0..7
#define CAN_INV_ID_MASK (0xff)
// CAN address - PB0..7
#define READ_INV_CAN_ADDR()     (GPIOB->IDR & 0xff)

extern uint16_t CANID; // self CAN ID (read @ init)

extern volatile uint32_t Tms;

void gpio_setup();
void iwdg_setup();
void tim1_setup();
void pause_ms(uint32_t pause);
#ifdef EBUG
void Jump2Boot();
#endif

#endif // __HARDWARE_H__
