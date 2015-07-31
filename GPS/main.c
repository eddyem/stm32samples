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

volatile uint32_t Timer = 0; // milliseconds
usbd_device *usbd_dev;
volatile uint32_t systick_val = 0;
volatile uint32_t timer_val = 0;
volatile int clear_ST_on_connect = 1;

curtime current_time = {25,61,61};

void time_increment(){
	Timer = 0;
	if(current_time.H == 25) return; // Time not initialized
	if(++current_time.S == 60){
		current_time.S = 0;
		if(++current_time.M == 60){
			current_time.M = 0;
			if(++current_time.H == 24)
				current_time.H = 0;
		}
	}
}

int main(){
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
		if((string = check_UART2())){
			GPS_parse_answer(string);
		}
		if(systick_val){
			P("Systick differs by ");
			print_int(systick_val);
			systick_val = 0;
			P(", timer value: ");
			print_int(timer_val);
			P("\n");
		}
	}
}


/**
 * SysTick interrupt: increment global time & send data buffer through USB
 */
void sys_tick_handler(){
	if(++Timer == 1000){
		time_increment();
	}
	usb_send_buffer();
}
// STK_CVR - current systick val
// STK_RVR - ticks till interrupt - 1

// PA4 interrupt
void exti4_isr(){
	if(EXTI_PR & EXTI4){
		// correct
		systick_val = STK_CVR;
		timer_val = Timer;
		if(clear_ST_on_connect){
			STK_CVR = 0;
			clear_ST_on_connect = 0;
			time_increment();
		}
		EXTI_PR = EXTI4;
	}
}


// pause function, delay in ms
void Delay(uint16_t _U_ time){
	uint32_t waitto = Timer + time;
	while(Timer < waitto);
}

/**
 * set current time by buffer hhmmss
 */
void set_time(uint8_t *buf){
	inline uint8_t atou(uint8_t *b){
		return (b[0]-'0')*10 + b[1]-'0';
	}
	uint8_t H = atou(buf) + 3;
	if(H > 23) H -= 24;
	current_time.H = H;
	current_time.M = atou(&buf[2]);
	current_time.S = atou(&buf[4]);
}


void print_curtime(){
	int T = Timer;
	newline();
	if(current_time.H < 25 && GPS_status != GPS_WAIT){
		P("Current time: ");
		if(current_time.H < 10) usb_send('0');
		print_int(current_time.H); usb_send(':');
		if(current_time.M < 10) usb_send('0');
		print_int(current_time.M); usb_send(':');
		if(current_time.S < 10) usb_send('0');
		print_int(current_time.S); usb_send('.');
	/*	uint32_t millis = STK_CVR * 1000;
		millis /= STK_RVR;*/
		if(T < 100) usb_send('0');
		if(T < 10) usb_send('0');
		print_int(T);
		if(GPS_status == GPS_NOT_VALID) P(" (not valid)");
		newline();
	}else
		P("Waiting for satellites\n");
}
