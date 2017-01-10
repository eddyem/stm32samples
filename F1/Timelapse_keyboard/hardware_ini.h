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
 * Tim2 - ultrasonic
 * Tim4 - beeper
 */

//void tim4_init();
//void beep();
void GPIO_init();
void SysTick_init();

// yellow LEDs: PA11, PA12; Y1- - trigr, Y2 - PPS
#define LEDS_Y_PORT      GPIOA
#define LEDS_Y1_PIN      GPIO13
#define LEDS_Y2_PIN      GPIO15
// green LEDs: PB7, PB8; G1 - GPS rdy
#define LEDS_G_PORT      GPIOB
#define LEDS_G1_PIN      GPIO7
#define LEDS_G2_PIN      GPIO8
// red LEDs: PB6, PB5; R2 - power
#define LEDS_R_PORT      GPIOB
#define LEDS_R1_PIN      GPIO6
#define LEDS_R2_PIN      GPIO5
// beeper - PB9
#define BEEPER_PORT      GPIOB
#define BEEPER_PIN       GPIO9

/*
// beeper period (in microseconds) - approx 440 Hz
#define BEEPER_PERIOD    (2273)
// amount of beeper pulses (after this walue it will be off) - near 2seconds
#define BEEPER_AMOUNT    (880)
*/
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
