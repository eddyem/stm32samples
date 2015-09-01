/*
 * ultrasonic.c
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

#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>

#include "ultrasonic.h"
#include "usbkeybrd.h"
#include "main.h"

static usmode US_mode = US_MODE_OFF;
static int overcapture = 0;

#ifndef TIM_CCMR2_CC3S_IN_TI4
#define TIM_CCMR2_CC3S_IN_TI4		(2)
#endif
#ifndef TIM_CCMR2_CC4S_IN_TI4
#define TIM_CCMR2_CC4S_IN_TI4		(1 << 8)
#endif
/**
 * Init timer 2 (&remap channels 3&4 to five-tolerant ports)
 */
void tim2_init(){
	// Turn off JTAG & SWD, remap TIM2_CH3/TIM2_CH4 to PB10/PB11 (five tolerant)
	// don't forget about AFIO clock & PB clock!
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPBEN);
	gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF, AFIO_MAPR_TIM2_REMAP_PARTIAL_REMAP2);
	// setup PB10 & PB11
	// PB10 - Trig output - push/pull
	gpio_set_mode(GPIO_BANK_TIM2_PR2_CH3, GPIO_MODE_OUTPUT_10_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_TIM2_PR2_CH3);
//	gpio_set_mode(GPIO_BANK_TIM2_CH3, GPIO_MODE_OUTPUT_10_MHZ,
//		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_TIM2_CH3);
	// PB11 - Echo input - floating
//	gpio_set_mode(GPIO_BANK_TIM2_PR2_CH4, GPIO_MODE_INPUT,
//		GPIO_CNF_INPUT_FLOAT, GPIO_TIM2_PR2_CH4);
	rcc_periph_clock_enable(RCC_TIM2);
	timer_reset(TIM2);
	// timers have frequency of 1MHz -- 1us for one step
	// 36MHz of APB1
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	// 72MHz div 72 = 1MHz
	TIM2_PSC = 71;  // prescaler is (div - 1)
	TIM2_ARR = TRIG_T;
	TIM2_CCR3 = TRIG_L;
}

/**
 * Send Trig signal
 * return 0 if another measurement still in process
 */
void start_ultrasonic(){
	overcapture = 0;
	TIM2_CR1 = 0;
	//TIM2_CR1 = TIM_CR1_URS; // Turn off timer to reconfigure, dusable UEV by UG set
	TIM2_CCER = 0;
	TIM2_CCMR2 = 0;
	TIM2_DIER = 0;
	TIM2_SR = 0; // clear all flags
	TIM2_ARR = TRIG_T;
	TIM2_CCR3 = TRIG_L;
	// PWM_OUT for TIM2_CH3
	TIM2_CCMR2 = TIM_CCMR2_CC3S_OUT | TIM_CCMR2_OC3M_PWM1;
	// start measurement, active is high
	TIM2_CCER = TIM_CCER_CC3E;
	// enable CC3 IRQ
	TIM2_DIER = TIM_DIER_CC3IE;
	nvic_enable_irq(NVIC_TIM2_IRQ);
	US_mode = US_MODE_TRIG;
	// start timer in one-pulse mode without update event
	TIM2_CR1 = TIM_CR1_OPM | TIM_CR1_CEN | TIM_CR1_UDIS;
}

/**
 * Start measurement:
 * TIM2_CH3 will capture high level & TIM2_CH4 will capture low
 * The timer configured to 65536us to detect signal lost
 */
inline void run_measrmnt(){
	US_mode = US_MODE_WAIT;
	TIM2_CR1 = 0; // Turn off timer to reconfigure
	TIM2_DIER = 0;
	TIM2_CCER = 0;
	TIM2_CCMR2 = 0;
	TIM2_SR = 0; // clear all flags
	TIM2_ARR = MAX_MSRMNT_LEN;
	// TIM2_CH3 & TIM2_CH4 configured as inputs, TIM2_CH3 connected to CH4
	TIM2_CCMR2 = TIM_CCMR2_CC3S_IN_TI4 | TIM_CCMR2_CC4S_IN_TI4;
	// start
	TIM2_CCER = TIM_CCER_CC4P | TIM_CCER_CC4E | TIM_CCER_CC3E;
	// enable interrupts CH3, CH4 & update
	TIM2_DIER = TIM_DIER_CC3IE | TIM_DIER_CC4IE | TIM_DIER_UIE;
	// start timer in one-pulse mode
	TIM2_CR1 = TIM_CR1_OPM | TIM_CR1_CEN;
}

void tim2_isr(){
	// No signal
	if(TIM2_SR & TIM_SR_UIF){ // update interrupt
		TIM2_SR = 0;
		overcapture = 1;
		nvic_disable_irq(NVIC_TIM2_IRQ);
		TIM2_DIER = 0;
		US_mode = US_MODE_READY;
	}
	// end of Trig pulse or start of measurements
	if(TIM2_SR & TIM_SR_CC3IF){ // CCR ch3 interrupt
		if(US_mode == US_MODE_TRIG){ // triggered - run measurement
			run_measrmnt();
		}else if(US_mode == US_MODE_WAIT){
			US_mode = US_MODE_MEASUREMENT;
		}
		TIM2_SR &= ~TIM_SR_CC3IF;
	}
	if(TIM2_SR & TIM_SR_CC4IF){
		if(US_mode == US_MODE_MEASUREMENT){
			US_mode = US_MODE_READY;
			nvic_disable_irq(NVIC_TIM2_IRQ);
			TIM2_DIER = 0;
			TIM2_CR1 = 0; // turn off timer - we don't need it more
		}
		TIM2_SR &= ~TIM_SR_CC3IF;
	}
}

/**
 * Measure distance
 * return 1 if measurements done
 * set L to distance (in mm) or 0 in case of overcapture (no signal)
 */
int ultrasonic_get(uint32_t *L){
	uint32_t D;
	if(US_mode != US_MODE_READY)
		return 0;
	US_mode = US_MODE_DONE;
	if(!overcapture){
		D = ((uint32_t)(TIM2_CCR4 - TIM2_CCR3)) * 170;
		*L = D / 1000;
	}else *L = 0;
	return 1;
}

uint32_t last_us_val = 0;
void poll_ultrasonic(){
	uint32_t L;
	if(US_mode == US_MODE_OFF){
		start_ultrasonic();
		return;
	}//else if(ultrasonic_ms != DIDNT_TRIGGERED) return;
	if(ultrasonic_get(&L)){ // measurements done, check event
		if(!overcapture){
			if(!last_us_val){
				last_us_val = L;
			}else{
				uint32_t diff = (last_us_val > L) ? last_us_val - L : L - last_us_val;
				if(diff > MAX_LEN_DIFF){
					if(last_us_val > L){ // someone move in front of sensor
						ultrasonic_ms = Timer;
						memcpy(&ultrasonic_time, &current_time, sizeof(curtime));
						P("Pass! Was: ");
						print_int(last_us_val);
						P(", become: ");
						print_int(L);
						P("!!!\n");
					}else{ // free space - check for noices (signal should be at least 10ms)
						diff = (ultrasonic_ms < Timer) ? Timer - ultrasonic_ms : ultrasonic_ms - Timer;
						if(diff < ULTRASONIC_TIMEOUT)
							ultrasonic_ms = DIDNT_TRIGGERED;
					}
					last_us_val = L;
				}
			}
		}
		start_ultrasonic();
	}
}
