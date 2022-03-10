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
#include "dmagpio.h"
#include "lcd.h"
#include "registers.h"

volatile uint32_t Timer = 0; // global timer (milliseconds)
usbd_device *usbd_dev;

int main(){
	uint32_t Old_timer = 0;

	// RCC clocking: 8MHz oscillator -> 72MHz system
	rcc_clock_setup_in_hse_8mhz_out_72mhz();
	// SysTick is a system timer with 1ms period
	SysTick_init();
	// disable JTAG to use PA13/PA14
	gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF, 0);
	GPIO_init();

	usb_disconnect(); // turn off USB while initializing all

	// USB
	usbd_dev = USB_init();

	dmagpio_init();

	usb_connect(); // turn on USB

	LCD_reset();

	int oldusblen = 0, x = 0, y = 0;
	int id = LCD_init();
	if(!id) P("failed to init LCD\n");
	uint16_t colr = 0;
	while(1){
		usbd_poll(usbd_dev);
		if(usbdatalen != oldusblen){ // there's something in USB buffer
			parse_incoming_buf(usbdatabuf, &usbdatalen);
			oldusblen = usbdatalen;
			id = 0;
		}
		if(transfer_complete){
			P("transfered\n");
			transfer_complete = 0;
		}
		setpix(x++, y++, colr);
		if(x > 200) x = 0;
		if(y > 200) y = 0;
		if(Timer - Old_timer > 999){ // one-second cycle
			Old_timer += 1000;
			P("Display id: ");
			print_hex((uint8_t*)&id, 2);
			newline();
			//write_reg(ILI932X_DISP_CTRL1, 0x0133);
			if(id){
				uint16_t r = read_reg(ILI932X_RW_GRAM);
				print_hex((uint8_t*)&r, 2);
				/*P(", status: ");
				r = LCD_status_read();
				print_hex((uint8_t*)&r, 2);*/
				newline();
				uint16_t i;
				for(i = 0; i < 0x29; ++i){
					r = read_reg(i);
					print_hex((uint8_t*)&i, 2); P(" = ");
					print_hex((uint8_t*)&r, 2); usb_send(' ');
					r = read_reg(i); print_hex((uint8_t*)&r, 2);
					newline();
				}
				putpoint(colr);
				colr += 32;
			}else{
				LCD_reset();
				id = LCD_init();
				if(!id) P("failed to init LCD\n");
			}
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
