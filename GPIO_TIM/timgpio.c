/*
 * timgpio.c
 *
 * Copyright 2016 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include "timgpio.h"
#include "user_proto.h"

int transfer_complete = 0;
static uint8_t addr[128];
static uint32_t len = 0, curidx = 0;

void timgpio_init(){
	// init TIM2 & DMA1ch2 (TIM2UP)
	rcc_periph_clock_enable(RCC_TIM2);
	rcc_periph_clock_enable(RCC_DMA1);
	timer_reset(TIM2);
	// timer have frequency of 1MHz
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	// 72MHz main freq, 2MHz for timer
	TIM2_PSC = 0;
	TIM2_ARR = 35;
	TIM2_DIER = TIM_DIER_UDE | TIM_DIER_UIE;
	nvic_enable_irq(NVIC_TIM2_IRQ);
}

void tim2_isr(){
	if(TIM2_SR & TIM_SR_UIF){ // update interrupt
		GPIOA_ODR = addr[curidx];
		if(++curidx >= len){
			TIM2_CR1 &= ~TIM_CR1_CEN;
			transfer_complete = 1;
		}
		TIM2_SR = 0;
	}
}


void timgpio_transfer(uint8_t *databuf, uint32_t length){
	transfer_complete = 0;
	memcpy(addr, databuf, length);
	len = length;
	curidx = 0;
	TIM2_CR1 |= TIM_CR1_CEN; // run timer
}
