/*
 * sharp.c - functions for Sharp 2Y0A02 distance meter
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

#include "sharp.h"
#include "main.h"

int AWD_flag = 0;
uint16_t AWD_value = 0;

void init_sharp_sensor(){
	// Make sure the ADC doesn't run during config
	adc_off(ADC1);
	// enable ADC & PA0 clocking
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_ADC1EN | RCC_APB2ENR_IOPAEN);
	rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV4);
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO0);
	// set sample time: 239.5 cycles for better results
	ADC1_SMPR2 = 7;
	// continuous conv, enable
	ADC1_CR2 = ADC_CR2_CONT | ADC_CR2_ADON;
	// reset calibration registers & start calibration
	ADC1_CR2 |= ADC_CR2_RSTCAL;
	while(ADC1_CR2 & ADC_CR2_RSTCAL); // wait for registers reset
	ADC1_CR2 |= ADC_CR2_CAL;
	while(ADC1_CR2 & ADC_CR2_CAL); // wait for calibration ends
	// set threshold limits
	ADC1_HTR = ADC_WDG_HIGH;
	ADC1_LTR = ADC_WDG_LOW;
	// enable analog watchdog on single regular channel 0 & enable interrupt
	ADC1_CR1 = ADC_CR1_AWDEN | ADC_CR1_AWDSGL | ADC_CR1_AWDIE;
	nvic_enable_irq(NVIC_ADC1_2_IRQ);
	ADC1_CR2 |= ADC_CR2_SWSTART;
	// start - to do it we need set ADC_CR2_ADON again!
	ADC1_CR2 |= ADC_CR2_ADON;
	DBG("ADC started\n");
}

void adc1_2_isr(){
	AWD_value = ADC1_DR;
	if(ADC1_SR & ADC_SR_AWD){ // analog watchdog event
		AWD_flag = 1;
	//	ADC1_CR1 &= ~(ADC_CR1_AWDIE | ADC_CR1_AWDEN);
	//	nvic_disable_irq(NVIC_ADC1_2_IRQ);
		if(AWD_value >= ADC_WDG_HIGH){ // high threshold
			ADC1_HTR = 0x0fff; // remove high threshold, only wait for LOW
			ADC1_LTR = ADC_WDG_LOW;
		}else{
			ADC1_HTR = ADC_WDG_HIGH;
			ADC1_LTR = 0;
		}
		//ADC1_CR1 |= ADC_CR1_AWDIE;
	}
	ADC1_SR = 0;
}
