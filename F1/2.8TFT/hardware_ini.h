/*
 * hardware_ini.h
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
#ifndef __HARDWARE_INI_H__
#define __HARDWARE_INI_H__

/*
 * Timers:
 * SysTick - system time
 */


void GPIO_init();
void SysTick_init();

/*
 * DMA to GPIO port & pins (mask) - PA0..PA7
 */
#define DMAGPIO_PORT		GPIOA
#define DMAGPIO_PINS		0xff
#define DMAGPIO_TARGADDR	((uint32_t)&(GPIOA_ODR))

#define LCD_write()		do{gpio_set_mode(DMAGPIO_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, DMAGPIO_PINS);}while(0)
//#define LCD_read()		do{gpio_set_mode(DMAGPIO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, DMAGPIO_PINS);}while(0)
#define LCD_read()		do{gpio_set_mode(DMAGPIO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, DMAGPIO_PINS); GPIO_ODR(DMAGPIO_PORT) = 0;}while(0)
// LCD control pins (PB10..PB14)
#define LCD_CONTROL_PORT	GPIOB
#define LCD_CS_PIN			GPIO12
#define LCD_RS_PIN			GPIO11
#define LCD_WR_PIN			GPIO14
#define LCD_RD_PIN			GPIO13
#define LCD_RST_PIN			GPIO10

// macro for control pins manipulation
#define CS_set				do{GPIO_BSRR(LCD_CONTROL_PORT) = LCD_CS_PIN;}while(0)
#define RS_set				do{GPIO_BSRR(LCD_CONTROL_PORT) = LCD_RS_PIN;}while(0)
#define RD_set				do{GPIO_BSRR(LCD_CONTROL_PORT) = LCD_RD_PIN;}while(0)
#define WR_set				do{GPIO_BSRR(LCD_CONTROL_PORT) = LCD_WR_PIN;}while(0)
#define CS_clear			do{GPIO_BSRR(LCD_CONTROL_PORT) = LCD_CS_PIN << 16;}while(0)
#define RS_clear			do{GPIO_BSRR(LCD_CONTROL_PORT) = LCD_RS_PIN << 16;}while(0)
#define RD_clear			do{GPIO_BSRR(LCD_CONTROL_PORT) = LCD_RD_PIN << 16;}while(0)
#define WR_clear			do{GPIO_BSRR(LCD_CONTROL_PORT) = LCD_WR_PIN << 16;}while(0)
#define RST_set				do{GPIO_BSRR(LCD_CONTROL_PORT) = LCD_RST_PIN;}while(0)
#define RST_clear			do{GPIO_BSRR(LCD_CONTROL_PORT) = LCD_RST_PIN << 16;}while(0)

#define LCD_wrbyte(x)		do{GPIO_ODR(DMAGPIO_PORT) = x;}while(0)
/*
#define LCD_wrbyte(x)		do{GPIO_BSRR(DMAGPIO_PORT) = DMAGPIO_PINS << 16; \
							GPIO_BSRR(DMAGPIO_PORT) = x;}while(0)
*/
#define LCD_rdbyte()		(GPIO_IDR(DMAGPIO_PORT) & DMAGPIO_PINS)

/*
 * USB interface
 * connect boot1 jumper to gnd, boot0 to gnd; and reconnect boot0 to +3.3 to boot flash
 */
/*
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
*/
// my simple devboard have no variants for programmed connection/disconnection of USB
#define usb_disconnect()
#define usb_connect()

#endif // __HARDWARE_INI_H__
