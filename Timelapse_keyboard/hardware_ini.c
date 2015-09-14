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
 * Init timer4 channel 4 (beeper)
 *
void tim4_init(){
	// setup PB9 - push/pull
	gpio_set_mode(GPIO_BANK_TIM4_CH4, GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_TIM4_CH4);
	rcc_periph_clock_enable(RCC_TIM4);
	timer_reset(TIM4);
	// timer have frequency of 1MHz to have ability of period changing with 1us discrete
	// 36MHz of APB1
	timer_set_mode(TIM4, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	// 72MHz div 72 = 1MHz
	TIM4_PSC = 71;  // prescaler is (div - 1)
	TIM4_ARR = BEEPER_PERIOD - 1;
	TIM4_CCR4 = BEEPER_PERIOD/2; // PWM 50/50%
	TIM4_DIER = TIM_DIER_UIE;
	// PWM_OUT for TIM4_CH4
	TIM4_CCMR2 = TIM_CCMR2_CC4S_OUT | TIM_CCMR2_OC4M_PWM1;
	nvic_enable_irq(NVIC_TIM4_IRQ);
}

uint32_t beeper_counter = 1;
*/
/**
 * Run beeper for BEEPER_AMOUNT pulses
 *
void beep(){
	TIM4_CR1 = 0; // stop timer if it was runned
	beeper_counter = BEEPER_AMOUNT;
	TIM4_SR = 0; // clear all flags
	TIM4_CR1 = TIM_CR1_CEN;
}

void tim4_isr(){
	// No signal
	if(TIM4_SR & TIM_SR_UIF){ // update interrupt
		TIM4_SR = 0;
		if(--beeper_counter == 0){ // done! Turn off timer
			TIM2_CR1 = 0;
		}
	}
}
*/

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
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO4 | GPIO5);
	//AFIO_EXTICR2 = 0;
	exti_enable_request(EXTI4 | EXTI5);
	// trigger on rising edge
	exti_set_trigger(EXTI4 | EXTI5, EXTI_TRIGGER_RISING);
	nvic_enable_irq(NVIC_EXTI4_IRQ);
	nvic_enable_irq(NVIC_EXTI9_5_IRQ);
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
