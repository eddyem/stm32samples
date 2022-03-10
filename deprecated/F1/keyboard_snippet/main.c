/*
 * main.c
 *
 * Copyright 2015 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include "ocm.h"
#include "usbkeybrd.h"

volatile uint32_t Timer = 0; // global timer (milliseconds)

/*
 * SysTick used for system timer with period of 1ms
 */
void SysTick_init(){
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8); // Systyck: 72/8=9MHz
	systick_set_reload(8999); // 9000 pulses: 1kHz
	systick_interrupt_enable();
	systick_counter_enable();
}

int main(void){
	uint32_t oldT = 0;

	rcc_clock_setup_in_hsi_out_48mhz();
	SysTick_init();
/*
	// if PC11 connected to usb 1.5kOhm pull-up through transistor
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set(GPIOC, GPIO11);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO11);
*/
	usbkeybrd_setup();
/*
	int i;
	for (i = 0; i < 0x80000; i++)
		__asm__("nop");
	gpio_clear(GPIOC, GPIO11);
*/
	uint32_t seconds_ctr = 0;
	while (1){
		poll_usbkeybrd();
		if(Timer - oldT > 999 || oldT > Timer){
			print_int(seconds_ctr++);
			newline();
			oldT = Timer;
		}
	}
}

/**
 * SysTick interrupt: increment global time & send data buffer through USB
 */
void sys_tick_handler(){
	Timer++;
	process_usbkbrd();
}
