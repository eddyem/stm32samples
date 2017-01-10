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
#include "hw_init.h"
#include "spi.h"
#include "cdcacm.h"
#include "pcd8544.h"

volatile uint32_t Timer = 0; // global timer (milliseconds)
usbd_device *usbd_dev;

/**
 * print current time in milliseconds: 4 bytes for ovrvlow + 4 bytes for time
 * with ' ' as delimeter
 */
void print_time(){
	print_int(Timer);
}


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

	switch_SPI(SPI1);
	SPI_init();
	pcd8544_init();

	usb_connect(); // turn on USB

	pcd8544_print((uint8_t*)"Все в порядке\n\nAll OK\n");

	uint8_t wb;
	int i;
	while(1){
		usbd_poll(usbd_dev);
		if(usbdatalen){
			for(i = 0; i < usbdatalen; ++i){
				uint8_t rb = (uint8_t)usbdatabuf[i];
				wb = pcd8544_putch(rb);
				if(wb == rb){
					pcd8544_roll_screen();
					wb = pcd8544_putch(rb);
				}
				if(wb != rb){
					usb_send(rb);
					if(wb == '\n') usb_send('\n');
				}
			}
			usbdatalen = 0;
		}
		//check_and_parse_UART(USART1); // also check data in UART buffers
		if(Timer - Old_timer > 999){ // one-second cycle
			Old_timer += 1000;
			//print_int(Timer / 1000);newline();
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
void Delay(uint16_t time){
	uint32_t waitto = Timer + time;
	while(Timer < waitto);
}
