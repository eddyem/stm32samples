/*
 * This file is part of the ws2815 project.
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

#include "stm32f1.h"

// LEDs amount in strip
#define LEDS_NUM    (60)

// LED0 - PC13 (bluepill), blinking each second
#define LED0_port   GPIOC
#define LED0_pin    (1<<13)

// USB pullup (not present in bluepill, should be soldered) - PA15
#define USBPU_port  GPIOA
#define USBPU_pin   (1<<15)
#define USBPU_ON()  pin_set(USBPU_port, USBPU_pin)
#define USBPU_OFF() pin_clear(USBPU_port, USBPU_pin)

#define LED_blink(x)    pin_toggle(x ## _port, x ## _pin)
#define LED_on(x)       pin_clear(x ## _port, x ## _pin)
#define LED_off(x)      pin_set(x ## _port, x ## _pin)

extern DMA_Channel_TypeDef * const WS2815DMAch;
extern DMA_TypeDef * const WS2815DMA;
#define WS2815TIM       TIM1
#define WS2815DMA_ISR_HTIF DMA_ISR_HTIF5
#define WS2815DMA_ISR_TCIF DMA_ISR_TCIF5
#define WS2815DMA_IFCR_CLR  DMA_IFCR_CGIF5

// defines for timer CCR1 values:
#define SHORTPULSE  (3)
#define LONGPULSE   (6)
// timer cc interrupt
#define WSTMCCISR   tim1_cc_isr
// DMA interrupt
#define WSDMAISR    dma1_channel5_isr

void hw_setup();
void iwdg_setup();
void startdata();
void stopdata();
void startreset();
void sendones();

#endif // __HARDWARE_H__
