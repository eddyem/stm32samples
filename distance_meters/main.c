/*
 * main.c
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

#include "main.h"
#include "hardware_ini.h"
#include "cdcacm.h"
#include "sharp.h"

volatile uint32_t Timer = 0; // global timer (milliseconds)
usbd_device *usbd_dev;

int main(){
	uint32_t Old_timer = 0;

	// RCC clocking: 8MHz oscillator -> 72MHz system
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	GPIO_init();

	usb_disconnect(); // turn off USB while initializing all

	// USB
	usbd_dev = USB_init();

	// SysTick is a system timer with 1ms period
	SysTick_init();
	tim2_init();
	//init_sharp_sensor();

	// wait a little and then turn on USB pullup
//	for (i = 0; i < 0x800000; i++)
//		__asm__("nop");

	usb_connect(); // turn on USB

	uint32_t oldL = 0;
	while(1){
		uint32_t L;
		usbd_poll(usbd_dev);
		if(usbdatalen){ // there's something in USB buffer
			usbdatalen = parce_incoming_buf(usbdatabuf, usbdatalen);
		}
		if(AWD_flag){
			P("Int, value = ");
			print_int(AWD_value);
			P(" ADU\n");
			AWD_flag = 0;
		}
		if(ultrasonic_get(&L)){
			if(!cont){
				P("Measured length: ");
				print_int(L);
				P("mm\n");
				oldL = 0;
			}else{
				if(!oldL){
					oldL = L;
				}else{
					uint32_t diff = (oldL > L) ? oldL - L : L - oldL;
					if(diff > MAX_LEN_DIFF){
						P("Pass! Was: ");
						print_int(oldL);
						P(", become: ");
						print_int(L);
						P("!!!\n");
						oldL = L;
					}
				}
				start_ultrasonic();
			}
		}
		if(Timer - Old_timer > 999){ // one-second cycle
			Old_timer += 1000;
		}else if(Timer < Old_timer){ // Timer overflow
			Old_timer = 0;
		}
	}
}


/**
 * SysTick interrupt: increment global time & send data buffer through USB
 */
void sys_tick_handler(){
	Timer++;
	usbd_poll(usbd_dev);
	usb_send_buffer();
}

// pause function, delay in ms
void Delay(uint16_t _U_ time){
	uint32_t waitto = Timer + time;
	while(Timer < waitto);
}

/**
 * print current time in milliseconds: 4 bytes for ovrvlow + 4 bytes for time
 * with ' ' as delimeter
 */
void print_time(){
	print_int(Timer);
}
