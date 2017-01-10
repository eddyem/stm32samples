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
 * Buttons "+"/"-"
 */
#define BTNS_PORT       GPIOA
// PA10 -- increase speed
#define BTN_PLUS_PIN      GPIO10
// PA9 -- decrease speed
#define BTN_MINUS_PIN      GPIO9

/*
 * Tacting out - PA4
 */
#define OUTP_PORT       GPIOA
#define OUTP_PIN        GPIO4

/*
 * LEDS: PA0 for bottom, PA1 for upper limits
 */
#define LEDS_PORT       GPIOA
#define LED_LOW_PIN     GPIO0
#define LED_UPPER_PIN   GPIO1

/*
 * USB interface
 * connect boot1 jumper to gnd, boot0 to gnd; and reconnect boot0 to +3.3 to boot flash
 */
/*
// USB_DICS (disconnect) - PC11
#define USB_DISC_PIN        GPIO11
#define USB_DISC_PORT       GPIOC
// USB_POWER (high level when USB connected to PC)
#define USB_POWER_PIN       GPIO10
#define USB_POWER_PORT      GPIOC
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
