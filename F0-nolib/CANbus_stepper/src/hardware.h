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
// DIR - PA4
#define SET_DIR()       do{GPIOA->BSRR = 1<<4;}while(0)
#define CLEAR_DIR()     do{GPIOA->BRR = 1<<4;}while(0)
// read ~FAULT - PF1 (inverse)
#define FAULT_STATE()   ((GPIOF->IDR & (1<<1)) ? 0 : 1)
// ~SLEEP - PC15 (inverse)
#define SLEEP_ON()      do{GPIOC->BRR = 1<<15;}while(0)
#define SLEEP_OFF()     do{GPIOC->BSRR = 1<<15;}while(0)
// configure ~SLP as PP output
#define SLP_CFG_OUT()   do{GPIOC->MODER = (GPIOC->MODER&~GPIO_MODER_MODER15) | GPIO_MODER_MODER15_O; }while(0)
// and ~SLP as floating input
#define SLP_CFG_IN()    do{GPIOC->MODER = (GPIOC->MODER&~GPIO_MODER_MODER15);}while(0)
// SLP state when input (non-inverted)
#define SLP_STATE()     ((GPIOC->IDR & (1<<15)) ? 1 : 0)
// ~EN, CFG6 - PC13 (inverse)
#define DRV_ENABLE()    do{GPIOC->BRR = 1<<13;}while(0)
#define DRV_DISABLE()   do{GPIOC->BSRR = 1<<13;}while(0)
// configure ~EN as PP output
#define EN_CFG_OUT()    do{GPIOC->MODER = (GPIOC->MODER&~GPIO_MODER_MODER13) | GPIO_MODER_MODER13_O; }while(0)
// configure ~EN as floating input
#define EN_CFG_IN()     do{GPIOC->MODER = (GPIOC->MODER&~GPIO_MODER_MODER13);}while(0)
// EN state when it's an input (non-inverted)
#define EN_STATE()      ((GPIOC->IDR & (1<<13)) ? 1 : 0)
// ~CS, CFG3, microstepping2 - PC14
#define SET_UST2()      do{GPIOC->BSRR = 1<<14;}while(0)
#define RESET_UST2()    do{GPIOC->BRR = 1<<14;}while(0)
#define CS_ACTIVE()     do{GPIOC->BRR = 1<<14;}while(0)
#define CS_PASSIVE()    do{GPIOC->BSRR = 1<<14;}while(0)
// configure ~CS as PP output
//#define CS_CFG_OUT()    do{GPIOC->MODER = (GPIOC->MODER&~GPIO_MODER_MODER14) | GPIO_MODER_MODER14_O; }while(0)
// ~CS as floating input
;
// Vio_ON, PF0 (inverse)
#define VIO_ON()        do{GPIOF->BRR = 1;}while(0)
#define VIO_OFF()       do{GPIOF->BSRR = 1;}while(0)

extern volatile uint32_t Tms;

void Jump2Boot();
void gpio_setup();
void iwdg_setup();
uint8_t getBRDaddr();
uint8_t refreshBRDaddr();
void sleep(uint16_t ms);
#endif // __HARDWARE_H__
