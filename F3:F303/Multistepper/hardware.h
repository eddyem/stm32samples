/*
 * This file is part of the multistepper project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32f3.h>

// "PCLK" frequency
// really APB1=36M -> TIM2/3/4/6/7 f=72M, APB2=72M -> TIM1/8/15/16/17/20 f=72M
#ifndef PCLK
#define PCLK    (72000000)
#endif

// motors' timer PSC = PCLK/Tfreq - 1, Tfreq=16MHz
#define MOTORTIM_PSC    (2)
// minimal ARR value - 99 for 5000 steps per second @ 32 microsteps/step
#define MOTORTIM_ARRMIN (99)

// USB pullup: PA8
#define USBPU_port  GPIOA
#define USBPU_pin   (1<<8)
// Pullup by P-channel MOSFET
#define USBPU_ON()  pin_clear(USBPU_port, USBPU_pin)
#define USBPU_OFF() pin_set(USBPU_port, USBPU_pin)
// direct pullup connected to pin
//#define USBPU_ON()  pin_set(USBPU_port, USBPU_pin)
//#define USBPU_OFF() pin_clear(USBPU_port, USBPU_pin)

// temporary LED: PD9
#define LED_blink() pin_toggle(GPIOD, 1<<9)

// Buttons amount
#define BTNSNO      (6)
// Buttons ports & pins
extern volatile GPIO_TypeDef* const BTNports[BTNSNO];
extern const uint32_t BTNpins[BTNSNO];
// state 1 - pressed, 0 - released (pin active is zero)
#define BTN_state(x)    ((BTNports[x]->IDR & BTNpins[x]) ? 0 : 1)

// motors:
#define MOTORSNO    (8)
extern volatile GPIO_TypeDef *ENports[MOTORSNO];
extern const uint32_t ENpins[MOTORSNO];
#define MOTOR_EN(x)  do{ pin_clear(ENports[x], ENpins[x]); }while(0)
#define MOTOR_DIS(x) do{ pin_set(ENports[x], ENpins[x]); }while(0)
extern volatile GPIO_TypeDef *DIRports[MOTORSNO];
extern const uint32_t DIRpins[MOTORSNO];
#define MOTOR_CW(x)  do{ pin_set(DIRports[x], DIRpins[x]); }while(0)
#define MOTOR_CCW(x) do{ pin_clear(DIRports[x], DIRpins[x]); }while(0)
// interval of velocity checking (10ms)
#define MOTCHKINTERVAL  (10)
// maximal ticks of encoder per step
#define MAXENCTICKSPERSTEP  (100)
// amount of full steps per revolution
#define STEPSPERREV         (200)

// Limit switches: 2 for each motor
#define ESWNO       (2)
// ESW ports & pins
extern volatile GPIO_TypeDef *ESWports[MOTORSNO][ESWNO];
extern const uint32_t ESWpins[MOTORSNO][ESWNO];

// extpins amount
#define EXTNO       (3)
extern volatile GPIO_TypeDef *EXTports[EXTNO];
extern const uint32_t   EXTpins[EXTNO];
#define EXT_SET(x)      do{ pin_set(EXTports[x], EXTpins[x]); }while(0)
#define EXT_CLEAR(x)    do{ pin_clear(EXTports[x], EXTpins[x]); }while(0)
#define EXT_TOGGLE(x)   do{ pin_toggle(EXTports[x], EXTpins[x]); }while(0)
#define EXT_CHK(x)      (pin_read(EXTports[x], EXTpins[x]))

// DIAG output of motors (PE2) & its multiplexer (PE3 - 0, PE4 - 1, PE5 - 2)
#define DIAG()          (GPIOE->IDR & (1<<2) ? 0 : 1)
#define DIAGMUL(x)      do{ register uint32_t v = x&7; GPIOE->BSRR = (v | (((~v)&7)<<16))<<3; }while(0)
#define DIAGMULCUR()    ((GPIOE->ODR >> 3) & 7)

extern volatile TIM_TypeDef *mottimers[MOTORSNO];

extern volatile uint32_t Tms;

uint8_t ESW_state(uint8_t MOTno);
uint8_t MSB(uint16_t val);
void hw_setup();
void mottimers_setup();
