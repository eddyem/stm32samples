/*
 * This file is part of the Stepper project.
 * Copyright 2020 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
// Most of hardware-dependendent definitions & functions
#include <stm32f0.h>

// refresh encoder data each 70ms
#define ENCODER_PERIOD  (69)

#define CONCAT(a,b)     a ## b
#define STR_HELPER(s)   #s
#define STR(s)          STR_HELPER(s)

#ifndef USARTNUM
#error "Define USARTNUM 1 or 2"
#endif
#define FORMUSART(X)   CONCAT(USART, X)
#define USARTX  FORMUSART(USARTNUM)

// Board address - PB15|14|13|12
#define READ_BRD_ADDR()  ((GPIOB->IDR >> 12) & 0x0f)

// RS-485 receive/transmit (PA8: 0-Rx, 1-Tx)
#define RS485_TX()      do{GPIOA->BSRR = GPIO_BSRR_BS_8;}while(0)
#define RS485_RX()      do{GPIOA->BRR  = GPIO_BRR_BR_8;}while(0)

// pins manipulation
// end-switches state
#define ESW_STATE()     ((GPIOB->IDR & 0x07) | ((GPIOB->IDR>>7) & 0x08))

extern volatile uint32_t Tms;

void Jump2Boot();
void gpio_setup();
void iwdg_setup();
uint8_t getBRDaddr();
uint8_t refreshBRDaddr();
void sleep(uint16_t ms);
//void timer_setup();
void tim2_Setup();

#endif // __HARDWARE_H__
