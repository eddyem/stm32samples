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
#include "ultrasonic.h"
#include "adc.h"

volatile uint32_t Timer = 0; // milliseconds
volatile uint32_t msctr = 0; // global milliseconds for different purposes
usbd_device *usbd_dev;
volatile int32_t systick_val = 0;
volatile int32_t timer_val = 0;
volatile int clear_ST_on_connect = 1;

volatile int need_sync = 1;

volatile uint32_t last_corr_time = 0; // time of last PPS correction (seconds from midnight)

// STK_CVR values for all milliseconds (RVR0) and last millisecond (RVR1)
volatile uint32_t RVR0 = STK_RVR_DEFAULT_VAL, RVR1 = STK_RVR_DEFAULT_VAL;

curtime current_time = {25,61,61};

curtime trigger_time = {25, 61, 61};
curtime adc_time[ADC_CHANNEL_NUMBER] = {{25, 61, 61}, {25, 61, 61}};
curtime ultrasonic_time = {25, 61, 61};
uint32_t trigger_ms = DIDNT_TRIGGERED, adc_ms[ADC_CHANNEL_NUMBER] = {DIDNT_TRIGGERED, DIDNT_TRIGGERED},
	ultrasonic_ms = DIDNT_TRIGGERED;

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
	int i;
	// RCC clocking: 8MHz oscillator -> 72MHz system
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	GPIO_init();

	usb_disconnect(); // turn off USB while initializing all

	// USB
	usbd_dev = USB_init();

	// SysTick is a system timer with 1ms period
	SysTick_init();
	UART_init(USART2); // init GPS UART

	tim2_init();
	// wait a little and then turn on USB pullup
//	for (i = 0; i < 0x800000; i++)
//		__asm__("nop");

	usb_connect(); // turn on USB

	GPS_send_start_seq();
	init_adc_sensor();

	uint32_t trigrtm = 0, adctm[2] = {0, 0}, ultrasonictm = 0;
	while(1){
		usbd_poll(usbd_dev);
		poll_ultrasonic();
		poll_ADC();
		if(usbdatalen){ // there's something in USB buffer
			usbdatalen = parse_incoming_buf(usbdatabuf, usbdatalen);
		}
		if((string = check_UART2())){
		//	P(string);
			GPS_parse_answer(string);
		}
		if(systick_val > 1000){
			P("Systick differs by ");
			print_int(systick_val);
			systick_val = 0;
			P(", timer value: ");
			print_int(timer_val);
			P(", RVR0 = ");
			print_int(RVR0);
			P(", RVR1 = ");
			print_int(RVR1);
			P("\n");
			print_curtime();
		}
		if(trigger_ms != DIDNT_TRIGGERED && trigger_ms != Timer){
			if(msctr - trigrtm > 500 || trigrtm > msctr){
				trigrtm = msctr;
				P("Trigger time: ");
				print_time(&trigger_time, trigger_ms);
			}
			trigger_ms = DIDNT_TRIGGERED;
		}
		for(i = 0; i < ADC_CHANNEL_NUMBER; ++i){
			if(adc_ms[i] != DIDNT_TRIGGERED && adc_ms[i] != Timer){
				if(msctr - adctm[i] > 500 || adctm[i] > msctr){
					/*int16_t diff = ADC_trig_val[i] - ADC_value[i];
					if(diff < 0) diff = -diff;
					if(diff < ADC_WDG_THRES/2){ // omit noice
					*/
					adctm[i] = msctr;
					P("ADC");
					usb_send('0'+i);
					if(adc_status[i] == ADWD_HI) P("hi");
					else if(adc_status[i] == ADWD_LOW) P("lo");
					P(": value = ");
					print_int(ADC_trig_val[i]);
					P(" (now: ");
					print_int(ADC_value[i]);
					P("), time = ");
					print_time(&adc_time[i], adc_ms[i]);
					//}
				}
				adc_ms[i] = DIDNT_TRIGGERED;
			}
		}
		if(ultrasonic_ms != DIDNT_TRIGGERED && ultrasonic_ms != Timer){
			if(msctr - ultrasonictm > 500 || ultrasonictm > msctr){
				ultrasonictm = msctr;
				P("Ultrasonic time: ");
				print_time(&ultrasonic_time, ultrasonic_ms);
			}
			ultrasonic_ms = DIDNT_TRIGGERED;
		}
	}
}


/**
 * SysTick interrupt: increment global time & send data buffer through USB
 */
void sys_tick_handler(){
	++Timer;
	++msctr;
	if(Timer == 999){
		STK_RVR = RVR1;
	}else if(Timer == 1000){
		STK_RVR = RVR0;
		time_increment();
	}
	usb_send_buffer();
}
// STK_CVR - current systick val
// STK_RVR - ticks till interrupt - 1

// PA4 interrupt - PPS signal
void exti4_isr(){
	uint32_t t = 0, ticks;
	static uint32_t ticksavr = 0, N = 0;
	if(EXTI_PR & EXTI4){
		// correct
		systick_val = STK_CVR;
		STK_CVR = RVR0;
		timer_val = Timer;
		Timer = 0;
		systick_val = STK_RVR + 1 - systick_val; // Systick counts down!
		if(timer_val < 10) timer_val += 1000; // our closks go faster than real
		else if(timer_val < 990){ // something wrong
			RVR0 = RVR1 = STK_RVR_DEFAULT_VAL;
			STK_RVR = RVR0;
			need_sync = 1;
			goto theend;
		}else
			time_increment(); // ms counter less than 1000 - we need to increment time
		t = current_time.H * 3600 + current_time.M * 60 + current_time.S;
		if(clear_ST_on_connect){
			clear_ST_on_connect = 0;
		}else{
			//  || (last_corr_time == 86399 && t == 0)
			if(t - last_corr_time == 1){ // PPS interval == 1s
				ticks =  systick_val + (timer_val-1)*(RVR0 + 1) + RVR1 + 1;
				++N;
				ticksavr += ticks;
				if(N > 20){
					ticks = ticksavr / N;
					RVR0 = ticks / 1000 - 1; // main RVR value
					STK_RVR = RVR0;
					RVR1 = RVR0 + ticks % 1000; // last millisecond RVR value (with fine correction)
					N = 0;
					ticksavr = 0;
					need_sync = 0;
				}
			}else{
				N = 0;
				ticksavr = 0;
			}
		}
	theend:
		last_corr_time = t;
		EXTI_PR = EXTI4;
	}
}

// pause function, delay in ms
void Delay(uint16_t time){
	uint32_t waitto = msctr + time;
	while(msctr != waitto);
}

/**
 * set current time by buffer hhmmss
 */
void set_time(uint8_t *buf){
	inline uint8_t atou(uint8_t *b){
		return (b[0]-'0')*10 + b[1]-'0';
	}
	uint8_t H = atou(buf) + TIMEZONE_GMT_PLUS;
	if(H > 23) H -= 24;
	current_time.H = H;
	current_time.M = atou(&buf[2]);
	current_time.S = atou(&buf[4]);
}

/**
 * print time: Tm - time structure, T - milliseconds
 */
void print_time(curtime *Tm, uint32_t T){
	int S = Tm->S, M = Tm->M, H = Tm->H;
	if(H < 10) usb_send('0');
	print_int(H); usb_send(':');
	if(M < 10) usb_send('0');
	print_int(M); usb_send(':');
	if(S < 10) usb_send('0');
	print_int(S); usb_send('.');
	if(T < 100) usb_send('0');
	if(T < 10) usb_send('0');
	print_int(T);
	if(GPS_status == GPS_NOT_VALID) P(" (not valid)");
	if(need_sync) P(" need synchronisation");
	newline();
}

void print_curtime(){
	uint32_t T = Timer;
	if(current_time.H < 24 && GPS_status != GPS_WAIT){
		P("Current time: ");
		print_time(&current_time, T);
	}else
		P("Waiting for satellites\n");
}
