/*
 * hw_init.h
 *
 * Copyright 2015 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#ifndef __HW_INIT_H__
#define __HW_INIT_H__

#include "main.h"

/*
 * USB interface
 */
// USB_DICS (disconnect) - PC11
#define USB_DISC_PIN		GPIO11
#define USB_DISC_PORT		GPIOC
// USB_POWER (high level when USB connected to PC)
#define USB_POWER_PIN		GPIO10
#define USB_POWER_PORT		GPIOC
// change signal level on USB diconnect pin
#define usb_disc_high()   gpio_set(USB_DISC_PORT, USB_DISC_PIN)
#define usb_disc_low()    gpio_clear(USB_DISC_PORT, USB_DISC_PIN)
// in case of n-channel FET on 1.5k pull-up change on/off disconnect means low level
// in case of pnp bipolar transistor or p-channel FET on 1.5k pull-up disconnect means high level
#define usb_disconnect()  usb_disc_high()
#define usb_connect()     usb_disc_low()

void GPIO_init();
void SysTick_init();

/* here are functions & macros for changing command pins state:
 *   SET_DC()    - set D/~C pin high (data)
 *   CLEAR_DC()  - clear D/~C (command)
 *   CHIP_EN()   - clear ~SCE
 *   CHIP_DIS()  - set ~SCE (disable chip)
 *   CLEAR_RST() - set 1 on RST pin
 *   LCD_RST()   - set 0 on RST pin
 */
/* pins:
 * DC:  PB6
 * SCE: PB7
 * RST: PB8
 */
#define DC_PORT   GPIOB
#define SCE_PORT  GPIOB
#define RST_PORT  GPIOB
#define DC_PIN    GPIO6
#define SCE_PIN   GPIO7
#define RST_PIN   GPIO8

#define SET_DC()    do{GPIO_BSRR(DC_PORT) = DC_PIN;}while(0)
#define CLEAR_DC()  do{GPIO_BSRR(DC_PORT) = DC_PIN << 16;}while(0)
#define CHIP_EN()   do{GPIO_BSRR(SCE_PORT) = SCE_PIN << 16;}while(0)
#define CHIP_DIS()  do{GPIO_BSRR(SCE_PORT) = SCE_PIN;}while(0)
#define CLEAR_RST() do{GPIO_BSRR(RST_PORT) = RST_PIN;}while(0)
#define LCD_RST()   do{GPIO_BSRR(RST_PORT) = RST_PIN << 16;}while(0)

#endif // __HW_INIT_H__
