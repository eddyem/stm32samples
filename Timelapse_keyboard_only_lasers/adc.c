/*
 * adc.c - functions for Sharp 2Y0A02 distance meter & photosensor
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

#include "adc.h"
#include "main.h"
#include "usbkeybrd.h"

uint16_t ADC_value;    // Value of ADC
uint16_t ADC_trig_val; // -//- at trigger time

void init_adc_sensor(){
	// we will use ADC1 channel 0 for IR sensor & ADC1 channel 1 for laser's photoresistor
	// Make sure the ADC doesn't run during config
	adc_off(ADC1);
	// enable ADC & PA0/PA1 clocking
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_ADC1EN | RCC_APB2ENR_IOPAEN);
	rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV4);
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO6);
	rcc_periph_clock_enable(RCC_DMA1); // enable DMA for ADC values storing
	// Configure ADC as continuous scan mode with DMA
	ADC1_CR1 = ADC_CR1_SCAN; // enable scan mode
	// set sample time on channels 1&2: 239.5 cycles for better results
	ADC1_SMPR2 = 0x3f;
	dma_channel_reset(DMA1, DMA_CHANNEL1);
	DMA1_CPAR1 = (uint32_t) &(ADC_DR(ADC1));
	DMA1_CMAR1 = (uint32_t) &ADC_value;
	DMA1_CNDTR1 = 1;
	DMA1_CCR1 = DMA_CCR_MINC | DMA_CCR_PSIZE_16BIT | DMA_CCR_MSIZE_16BIT
			| DMA_CCR_CIRC | DMA_CCR_PL_HIGH | DMA_CCR_EN;
	// continuous conv, enable ADC & DMA
	ADC1_CR2 = ADC_CR2_CONT | ADC_CR2_ADON | ADC_CR2_DMA;
	// set channels
	uint8_t adc_channel_array = ADC_POWER_CHANNEL;
	adc_set_regular_sequence(ADC1, 1, &adc_channel_array);
	// reset calibration registers & start calibration
	ADC1_CR2 |= ADC_CR2_RSTCAL;
	while(ADC1_CR2 & ADC_CR2_RSTCAL); // wait for registers reset
	ADC1_CR2 |= ADC_CR2_CAL;
	while(ADC1_CR2 & ADC_CR2_CAL); // wait for calibration ends
	nvic_enable_irq(NVIC_ADC1_2_IRQ);
	ADC1_CR2 |= ADC_CR2_SWSTART;
	// turn on ADC - to do it we need set ADC_CR2_ADON again!
	ADC1_CR2 |= ADC_CR2_ADON;
}
