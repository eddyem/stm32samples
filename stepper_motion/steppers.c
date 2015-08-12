/*
 * steppers.c
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

#include "steppers.h"
#include "user_proto.h"
#include <libopencm3/stm32/timer.h>


volatile int32_t Glob_steps = 0;
volatile int Dir = 0; // 0 - stop, 1 - move to +, -1 - move to -
int32_t stepper_period = STEPPER_DEFAULT_PERIOD;

/**
 * Init TIM2_CH3 (& remap to 5V tolerant PB10) [STEP] and PB11 [DIR]
 */
void steppers_init(){
	// Turn off JTAG & SWD, remap TIM2_CH3 to PB10 (five tolerant)
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPBEN);
	gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF, AFIO_MAPR_TIM2_REMAP_PARTIAL_REMAP2);
	// setup PB10 & PB11 - both opendrain
	gpio_set_mode(GPIO_BANK_TIM2_PR2_CH3, GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, GPIO_TIM2_PR2_CH3);
	//	GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_TIM2_PR2_CH3);
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_OPENDRAIN, GPIO11);
	rcc_periph_clock_enable(RCC_TIM2);
	timer_reset(TIM2);
	// timers have frequency of 1MHz -- 1us for one step
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	// 36MHz of APB1 x2 = 72MHz
	TIM2_PSC = STEPPER_TIM_DEFAULT_PRESCALER;  // prescaler is (div - 1)
	TIM2_CR1 = 0;
	TIM2_CCER = 0;
	TIM2_CCMR2 = 0;
	TIM2_DIER = 0;
	TIM2_SR = 0; // clear all flags
	// PWM_OUT for TIM2_CH3
	TIM2_CCMR2 = TIM_CCMR2_CC3S_OUT | TIM_CCMR2_OC3M_PWM1;
	// active is low
	TIM2_CCER = TIM_CCER_CC3E | TIM_CCER_CC3P;
	// enable update IRQ
	TIM2_DIER = TIM_DIER_UIE;
	TIM2_ARR  = STEPPER_DEFAULT_PERIOD;
	TIM2_CCR3 = STEPPER_PULSE_LEN;
	nvic_enable_irq(NVIC_TIM2_IRQ);
}

/**
 * Set stepper period
 */
uint8_t set_stepper_speed(int32_t Period){
	if(Period > STEPPER_MIN_PERIOD && Period < STEPPER_MAX_PERIOD){
		P("Set stepper period to ");
		if(Period > STEPPER_TIM_MAX_ARRVAL){ // redefine prescaler when value is too large
			TIM2_PSC = STEPPER_TIM_HUNDR_PRESCALER;
			Period /= 100;
			print_int(Period*100);
			TIM2_ARR = --Period;
		}else{ // default prescaler - for 1MHz
			print_int(Period);
			TIM2_PSC = STEPPER_TIM_DEFAULT_PRESCALER;
			TIM2_ARR = --Period;
		}
		stepper_period = Period;
		newline();
	}
	return 0;
}

void stop_stepper(){
	TIM2_CCR3 = 0;
	TIM2_CR1 = 0;
	Glob_steps = 0;
	Dir = 0;
	gpio_set(GPIOB, GPIO11); // clear "direction"
	P("Stopped!\n");
}

/**
 * move stepper motor to N steps
 * if motor is moving - just add Nsteps to current counter
 */
uint8_t move_stepper(int32_t Nsteps){
	int dir = 1;
	if(!Nsteps) return 0;
	if(TIM2_CR1){ // moving
		if(!gpio_get(GPIOB, GPIO11)) dir = 0; // negative direction
		Glob_steps += Nsteps;
		if(Glob_steps < 0){
			Glob_steps = -Glob_steps;
			dir = !dir;
		}
	}else{ // start timer if not moving
		if(Nsteps < 0){ // change direction
			dir = 0;
			Nsteps = -Nsteps;
		}
		Glob_steps = Nsteps;
		TIM2_ARR = stepper_period;
		TIM2_CCR3 = STEPPER_PULSE_LEN;
		TIM2_CR1 = TIM_CR1_CEN;
	}
	// set/clear direction
	P("Rotate ");
	if(dir){
		gpio_set(GPIOB, GPIO11);
	}else{
		gpio_clear(GPIOB, GPIO11);
		P("counter-");
	}
	P("clockwise to ");
	print_int(Glob_steps);
	P(" steps\n");
	return 0;
}

void tim2_isr(){
	// Next step
	if(TIM2_SR & TIM_SR_UIF){
		if(--Glob_steps < 1){ // stop timer as steps ends
			stop_stepper();
		}
	}
	TIM2_SR = 0;
}


int32_t stepper_get_period(){
	int32_t r = stepper_period + 1;
	if(TIM2_PSC == STEPPER_TIM_DEFAULT_PRESCALER) return r;
	else return r * 100;
}
