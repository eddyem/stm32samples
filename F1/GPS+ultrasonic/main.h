/*
 * main.h
 *
 * Copyright 2014 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 */


#pragma once
#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdlib.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/spi.h>

#include "sync.h" // mutexes
#include "user_proto.h"

extern void *memcpy(void *dest, const void *src, int n);

#define _U_    __attribute__((__unused__))
#define  U8(x)  ((uint8_t)  x)
#define U16(x)  ((uint16_t) x)
#define U32(x)  ((uint32_t) x)

#define STK_RVR_DEFAULT_VAL  (8999)
#define TIMEZONE_GMT_PLUS    (3)

#define DIDNT_TRIGGERED (2000)

typedef struct{
	uint8_t H;
	uint8_t M;
	uint8_t S;
} curtime;

extern curtime current_time;
extern volatile uint32_t Timer; // global timer (milliseconds)
extern volatile int clear_ST_on_connect; // flag for clearing Systick counter on next PPS

extern curtime trigger_time, adc_time[], ultrasonic_time;
extern uint32_t trigger_ms, adc_ms[], ultrasonic_ms;

extern volatile int need_sync;

void Delay(uint16_t time);
void set_time(uint8_t *buf);

void print_time(curtime *T, uint32_t m);
void print_curtime();

#endif // __MAIN_H__

