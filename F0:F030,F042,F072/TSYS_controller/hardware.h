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

#define CAN_SPEED_DEFAULT       (250)
#define CAN_SPEED_MIN           (12)
#define CAN_SPEED_MAX           (1000)

// LED0
#define LED0_port   GPIOB
#define LED0_pin    (1<<10)
// LED1
#define LED1_port   GPIOB
#define LED1_pin    (1<<11)

#ifndef USARTNUM
#define USARTNUM 2
#endif

#define CONCAT(a,b)     a ## b
#define STR_HELPER(s)   #s
#define STR(s) STR_HELPER(s)

#define FORMUSART(X)   CONCAT(USART, X)
#define USARTX  FORMUSART(USARTNUM)

#ifndef I2CPINS
#define I2CPINS 910
#endif

#ifndef LED1_port
#define LED1_port   LED0_port
#endif
#ifndef LED1_pin
#define LED1_pin    LED0_pin
#endif
#define LED_blink(x) pin_toggle(x ## _port, x ## _pin)
#define LED_on(x)    pin_clear(x ## _port, x ## _pin)
#define LED_off(x)   pin_set(x ## _port, x ## _pin)

// set active channel number
#define MUL_ADDRESS(x)  do{GPIOB->BSRR = (0x7 << 16) | (x);}while(0)
// address from 0 to 7
// WARNING!!! In current case all variables for sensors counting are uint8_t, so if
// MUL_MAX_ADDRESS would be greater than 7 you need to edit all codes!!!11111111111111111111
#define MUL_MAX_ADDRESS     (7)
// turn multiplexer on/off (PB12 -> 1/0)
#define MUL_ON()        pin_clear(GPIOB, (1<<12))
#define MUL_OFF()       pin_set(GPIOB, (1<<12))

// turn on/off power of sensors (PA8-> 1/0)
#define SENSORS_ON()    pin_set(GPIOA, (1<<8))
#define SENSORS_OFF()   pin_clear(GPIOA, (1<<8))
// check overcurrent (PB3 == 0)
#define SENSORS_OVERCURNT()  ((1<<3) != (GPIOB->IDR & (1<<3)))

// CAN address - PA13..PA15
#define READ_CAN_INV_ADDR()  (((GPIOA->IDR & (0x7<<13))>>13) | ((GPIOB->IDR & (1<<15)) >> 12))
extern uint8_t Controller_address;

typedef enum{
    VERYLOW_SPEED,
    LOW_SPEED,
    HIGH_SPEED,
    CURRENT_SPEED
} I2C_SPEED;

extern I2C_SPEED curI2Cspeed;

void gpio_setup(void);
void i2c_setup(I2C_SPEED speed);

#endif // __HARDWARE_H__
