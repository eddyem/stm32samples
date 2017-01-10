/*
 * dmagpio.c
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

#include "dmagpio.h"
#include "user_proto.h"

int transfer_complete = 0;
static uint16_t gpiobuff[128] = {0};

void dmagpio_init(){
	// init TIM2 & DMA1ch2 (TIM2UP)
	rcc_periph_clock_enable(RCC_TIM2);
	rcc_periph_clock_enable(RCC_DMA1);
	timer_reset(TIM2);
	// timer have frequency of 1MHz
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	// 72MHz div 18 = 4MHz
	TIM2_PSC = 0; // prescaler is (div - 1)
	TIM2_ARR = 1; // 36MHz (6.25)
	TIM2_DIER = TIM_DIER_UDE;// | TIM_DIER_UIE;
//nvic_enable_irq(NVIC_TIM2_IRQ);

	dma_channel_reset(DMA1, DMA_CHANNEL2);
	// mem2mem, medium prio, 8bits, memory increment, read from mem, transfer complete en
	//DMA1_CCR2 = DMA_CCR_MEM2MEM | DMA_CCR_PL_MEDIUM | DMA_CCR_MSIZE_16BIT |
	DMA1_CCR2 = DMA_CCR_PL_MEDIUM | DMA_CCR_MSIZE_16BIT |
		DMA_CCR_PSIZE_16BIT | DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE | DMA_CCR_TEIE ;
	nvic_enable_irq(NVIC_DMA1_CHANNEL2_IRQ);
	// target address:
	DMA1_CPAR2 = DMAGPIO_TARGADDR;
	DMA1_CMAR2 = (uint32_t) gpiobuff;
}
/*
void tim2_isr(){
	if(TIM2_SR & TIM_SR_UIF){ // update interrupt
		++cntr;
		GPIOA_ODR = gpiobuff[curidx];
		if(++curidx >= len){
			TIM2_CR1 &= ~TIM_CR1_CEN;
			transfer_complete = 1;
		}
		TIM2_SR = 0;
	}
}
*/

void dmagpio_transfer(uint8_t *databuf, uint32_t length){
	while(DMA1_CCR2 & DMA_CCR_EN);
	transfer_complete = 0;
	DMA1_IFCR = 0xff00; // clear all flags for ch2
	// memory address
	//DMA1_CMAR2 = (uint32_t) databuf;
	// buffer length
//	DMA1_CPAR2 = DMAGPIO_TARGADDR;
//	DMA1_CMAR2 = (uint32_t) gpiobuff;
	DMA1_CNDTR2 = length;
	uint32_t i;
	for(i = 0; i < length; ++i) gpiobuff[i] = databuf[i];
	TIM2_CR1 |= TIM_CR1_CEN; // run timer
	DMA1_CCR2 |= DMA_CCR_EN;
}

void dma1_channel2_isr(){
	if(DMA1_ISR & DMA_ISR_TCIF2){
		transfer_complete = 1;
		// stop timer & turn off DMA
		TIM2_CR1 &= ~TIM_CR1_CEN;
		DMA1_CCR2 &= ~DMA_CCR_EN;
		DMA1_IFCR = DMA_IFCR_CTCIF2; // clear flag
	/*	uint32_t arr = TIM2_ARR;
		if(arr == 1) TIM2_ARR = 71;
		else TIM2_ARR = arr - 1;*/
	}else if(DMA1_ISR & DMA_ISR_TEIF2){
		P("Error\n");
		DMA1_IFCR = DMA_IFCR_CTEIF2;
		TIM2_CR1 &= ~TIM_CR1_CEN;
		DMA1_CCR2 &= ~DMA_CCR_EN;
	}
}
