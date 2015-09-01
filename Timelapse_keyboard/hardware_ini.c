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

/**
 * GPIO initialisaion: clocking + pins setup
 */
void GPIO_init(){
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN |
			RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN |
			RCC_APB2ENR_IOPEEN);
	/*
	 * Setup EXTI on PA4 (PPS input from GPS) - pull down
	 * EXTI on PA5 - also pull down (trigger for time measurement)
	 */
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO4 | GPIO5);
	//AFIO_EXTICR2 = 0;
	exti_enable_request(EXTI4 | EXTI5);
	// trigger on rising edge
	exti_set_trigger(EXTI4 | EXTI5, EXTI_TRIGGER_RISING);
	nvic_enable_irq(NVIC_EXTI4_IRQ);
	nvic_enable_irq(NVIC_EXTI9_5_IRQ);
/*
	// Buttons: pull-up input
	gpio_set_mode(BTNS_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
			BTN_S2_PIN | BTN_S3_PIN);
	// turn on pull-up
	gpio_set(BTNS_PORT, BTN_S2_PIN | BTN_S3_PIN);
	// LEDS: opendrain output
	gpio_set_mode(LEDS_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN,
			LED_D1_PIN | LED_D2_PIN);
	// turn off LEDs
	gpio_set(LEDS_PORT, LED_D1_PIN | LED_D2_PIN);*/
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
 * PA5 interrupt - print time at button/switch trigger
 */
void exti9_5_isr(){
	if(EXTI_PR & EXTI5){
		if(trigger_ms == DIDNT_TRIGGERED){ // prevent bounce
			trigger_ms = Timer;
			memcpy(&trigger_time, &current_time, sizeof(curtime));
		}
		EXTI_PR = EXTI5;
	}
}
