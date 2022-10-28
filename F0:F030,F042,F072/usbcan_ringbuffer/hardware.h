/*
 * This file is part of the usbcanrb project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32f0.h>

#define CONCAT(a,b)     a ## b
#define STR_HELPER(s)   #s
#define STR(s)          STR_HELPER(s)

#define FORMUSART(X)    CONCAT(USART, X)
#define USARTX          FORMUSART(USARTNUM)

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


// CAN address - PB14(0), PB15(1), PA8(2)
#define READ_CAN_INV_ADDR()  (((GPIOA->IDR & (1<<8))>>6)|((GPIOB->IDR & (3<<14))>>14))


extern volatile uint32_t Tms;

extern uint8_t ledsON;

void gpio_setup(void);
void iwdg_setup();
void pause_ms(uint32_t pause);
void Jump2Boot();
