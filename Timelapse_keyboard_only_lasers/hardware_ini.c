/*
 * hardware_ini.c - functions for HW initialisation
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

/*
 * All hardware-dependent initialisation & definition should be placed here
 * and in hardware_ini.h
 *
 */

#include "main.h"
#include "hardware_ini.h"
#include <libopencm3/stm32/timer.h>

/**
 * GPIO initialisaion: clocking + pins setup
 */
void GPIO_init(){
	// enable clocking for all ports, APB2 & AFIO (we need AFIO to remap JTAG pins)
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN |
			RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN |
			RCC_APB2ENR_IOPEEN | RCC_APB2ENR_AFIOEN);
	// turn off SWJ/JTAG
	AFIO_MAPR = AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF;
	/*
	 * Setup EXTI on PA4 (PPS input from GPS) - pull down
	 * EXTI on PA5 - also pull down (trigger for time measurement)
	 */
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO0 | GPIO1 | GPIO4 | GPIO5 | GPIO7 | GPIO8);
	// PA0/1 and PA7/8 - Lasers
	//gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO0 | GPIO1);
	//gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO0 | GPIO1);
	// EXTI0 - Laser1, EXTI1 - Laser2, EXTI4 - PPS, EXTI5 - Button, EXTI7 - Laser3, EXTI8 - Laser4
	// trigger on rising edge
	exti_set_trigger(EXTI0 | EXTI1 | EXTI4 | EXTI5 | EXTI7 | EXTI8, EXTI_TRIGGER_RISING);
	AFIO_EXTICR1 = 0; // EXTI 0-3 from PORTA
	nvic_enable_irq(NVIC_EXTI4_IRQ); // PPS
	nvic_enable_irq(NVIC_EXTI0_IRQ); // Laser1
	nvic_enable_irq(NVIC_EXTI1_IRQ); // Laser2
	nvic_enable_irq(NVIC_EXTI9_5_IRQ); // Button, Lasers 3/4
	exti_enable_request(EXTI0 | EXTI1 | EXTI4 | EXTI5 | EXTI7 | EXTI8);
	// LEDS: opendrain output (&turn all OFF)
	gpio_set(LEDS_Y_PORT, LEDS_Y1_PIN | LEDS_Y2_PIN);
	gpio_set_mode(LEDS_Y_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN,
			LEDS_Y1_PIN | LEDS_Y2_PIN);
	gpio_set(LEDS_G_PORT, LEDS_G1_PIN | LEDS_G2_PIN);
	gpio_set_mode(LEDS_G_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN,
			LEDS_G1_PIN | LEDS_G2_PIN);
	gpio_set(LEDS_R_PORT, LEDS_R1_PIN | LEDS_R2_PIN);
	gpio_set_mode(LEDS_R_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN,
			LEDS_R1_PIN | LEDS_R2_PIN);
	// beeper pin: push-pull
	gpio_set(BEEPER_PORT, BEEPER_PIN);
	gpio_set_mode(BEEPER_PORT, GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL, BEEPER_PIN);
/*
	// USB_DISC: push-pull
	gpio_set_mode(USB_DISC_PORT, GPIO_MODE_OUTPUT_2_MHZ,
				GPIO_CNF_OUTPUT_PUSHPULL, USB_DISC_PIN);
	// USB_POWER: open drain, externall pull down with R7 (22k)
	gpio_set_mode(USB_POWER_PORT, GPIO_MODE_INPUT,
				GPIO_CNF_INPUT_FLOAT, USB_POWER_PIN);
*/
}

/**
 * PA5 interrupt - print time at button-switch trigger and Lasers3/4
 * EXTI5 - Button, EXTI7 - Laser3, EXTI8 - Laser4
 */
void exti9_5_isr(){
	if(EXTI_PR & EXTI5){
		if(trigger_ms[0] == DIDNT_TRIGGERED){ // prevent bounce
			trigger_ms[0] = Timer;
			memcpy(&trigger_time[0], &current_time, sizeof(curtime));
		}
		EXTI_PR = EXTI5;
	}
	if(EXTI_PR & EXTI7){
		if(trigger_ms[3] == DIDNT_TRIGGERED){ // prevent bounce
			trigger_ms[3] = Timer;
			memcpy(&trigger_time[3], &current_time, sizeof(curtime));
		}
		EXTI_PR = EXTI7;
	}
	if(EXTI_PR & EXTI8){
		if(trigger_ms[4] == DIDNT_TRIGGERED){ // prevent bounce
			trigger_ms[4] = Timer;
			memcpy(&trigger_time[4], &current_time, sizeof(curtime));
		}
		EXTI_PR = EXTI8;
	}
}

void exti0_isr(){ // Laser1
	if(EXTI_PR & EXTI0){
		if(trigger_ms[1] == DIDNT_TRIGGERED){ // prevent bounce
			trigger_ms[1] = Timer;
			memcpy(&trigger_time[1], &current_time, sizeof(curtime));
		}
		EXTI_PR = EXTI0;
	}
}

void exti1_isr(){ // Laser2
//	if(EXTI_PR & EXTI1){
		if(trigger_ms[2] == DIDNT_TRIGGERED){ // prevent bounce
			trigger_ms[2] = Timer;
			memcpy(&trigger_time[2], &current_time, sizeof(curtime));
		}
		EXTI_PR = EXTI1;
	//}
}
