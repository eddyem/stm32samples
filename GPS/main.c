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
#include "uart.h"
#include "GPS.h"

volatile uint32_t Ticks = 0; // global ticks (10kHz)
volatile uint32_t Timer = 0; // global timer (milliseconds)
usbd_device *usbd_dev;

int main(){
	uint32_t usb_timer = 0;
	uint8_t *string;
	// RCC clocking: 8MHz oscillator -> 72MHz system
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	GPIO_init();

	usb_disconnect(); // turn off USB while initializing all

	// USB
	usbd_dev = USB_init();

	// SysTick is a system timer with 1ms period
	SysTick_init();
	UART_init(USART2); // init GPS UART

	// wait a little and then turn on USB pullup
//	for (i = 0; i < 0x800000; i++)
//		__asm__("nop");

	usb_connect(); // turn on USB

	GPS_send_start_seq();

	while(1){
		usbd_poll(usbd_dev);
		if(usbdatalen){ // there's something in USB buffer
			usbdatalen = parce_incoming_buf(usbdatabuf, usbdatalen);
		}
		if((string = check_UART2()))
			GPS_parse_answer(string);
		if(Ticks - usb_timer > 9){ // 1ms cycle
			usb_timer += 10;
			//usb_send_buffer();
		}else if(Ticks < usb_timer){ // Timer overflow
			usb_timer = 0;
		}
	}
}


/**
 * SysTick interrupt: increment global time & send data buffer through USB
 */
void sys_tick_handler(){
	static int T = 0; // 0..10 - for millisecond timer
	++Ticks;
	if(++T == 10){
		usb_send_buffer();
		++Timer;
		T = 0;
	}
}

// pause function, delay in ms
void Delay(uint16_t _U_ time){
	uint32_t waitto = Timer + time;
	while(Timer < waitto);
}

/**
 * print current time in milliseconds
 */
void print_time(){
	print_int(Timer);
}
