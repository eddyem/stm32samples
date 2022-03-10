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

#include "stm32f1.h"
#include "screen.h"

// define this if screen colors are negative
// #define SCREEN_IS_NEGATIVE
// define if nOE is negative
#define NOE_IS_NEGATIVE

// LED0 - PC13 (bluepill), blinking each second
#define LED0_port   GPIOC
#define LED0_pin    (1<<13)

// USB pullup (not present in bluepill) - PA15
#define USBPU_port  GPIOA
#define USBPU_pin   (1<<15)
#define USBPU_ON()  pin_set(USBPU_port, USBPU_pin)
#define USBPU_OFF() pin_clear(USBPU_port, USBPU_pin)

#define LED_blink(x)    pin_toggle(x ## _port, x ## _pin)
#define LED_on(x)       pin_clear(x ## _port, x ## _pin)
#define LED_off(x)      pin_set(x ## _port, x ## _pin)

// SPI DMA channel
#define DMA_SPI_Channel DMA1_Channel3
// SCREEN PINs: A,B,C,D - PB6-PB9; LAT - PB5; nOE - PB4
// CLK - PA6 (TIM3_CH1)
// colors: PA0..PA5
#define COLR_port   GPIOA
#define COLR_pin    (0x3f)
#define ADDR_port   GPIOB
#define ADDR_roll   (6)
#define ADDR_pin    (0xf<<ADDR_roll)
#define LAT_port    GPIOB
#define LAT_pin     (1<<5)
#define nOE_port    GPIOB
#define nOE_pin     (1<<4)
#define SET(x)      pin_set(x ## _port, x ## _pin)
#define CLEAR(x)    pin_clear(x ## _port, x ## _pin)
#define TOGGLE(x)   pin_toggle(x ## _port, x ## _pin)

#ifdef NOE_IS_NEGATIVE
#define SCRN_ENBL()     SET(nOE)
#define SCRN_DISBL()    CLEAR(nOE)
#else
#define SCRN_ENBL()     CLEAR(nOE)
#define SCRN_DISBL()    SET(nOE)
#endif

void iwdg_setup();
void hw_setup();
void TIM_DMA_transfer(uint8_t blknum);
void stopTIMDMA();
void ConvertScreenBuf(uint8_t *sbuf, uint8_t Nrow, uint8_t Ntick);

extern uint8_t transfer_done;

#endif // __HARDWARE_H__
