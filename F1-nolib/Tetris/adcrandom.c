/*
 * This file is part of the TETRIS project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// random numbers generator based on ADC values of Tsens and Vref

#include "adcrandom.h"


#define ADC_CHANNELS_NO  (2)

// 16-Tsens, 17-Vref
static uint8_t const ADCchno[ADC_CHANNELS_NO] = {16, 17};

uint16_t getADCval(uint8_t ch){
    if(ch >= ADC_CHANNELS_NO) return 0;
    ADC1->SQR3 = ADCchno[ch];
    ADC1->SR = 0;
    // turn ON ADC
    ADC1->CR2 |= ADC_CR2_ADON;
    while(!(ADC1->SR & ADC_SR_EOC)); // wait end of conversion
    return ADC1->DR;
}

struct xorshift128_state {
  uint32_t a, b, c, d;
} state = {0};
static uint32_t xorshift128(){
	/* Algorithm "xor128" from p. 5 of Marsaglia, "Xorshift RNGs" */
	uint32_t t = state.d;
	uint32_t const s = state.a;
	state.d = state.c;
	state.c = state.b;
	state.b = s;

	t ^= t << 11;
	t ^= t >> 8;
	return state.a = t ^ s ^ (s >> 19);
}

static uint32_t rnd(){
    uint32_t r = 0;
    for(int i = 0; i < 32; ++i){
        r <<= 1;
        //r |= (getADCval(0) & 1) ^ (getADCval(1) & 1);
        r |= getADCval(0) & 1;
    }
    return r;
}

uint32_t getRand(){
    static uint8_t g = 0;
    if(g++ == 0){
        state.a = rnd();
        state.b = rnd();
        state.c = rnd();
        state.d = rnd();
    }
    return xorshift128();
}

void adc_setup(){
    //GPIOC->CRL |= CRL(0, CNF_ANALOG|MODE_INPUT);
    uint32_t ctr = 0;
    // Enable clocking
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->CFGR &= ~(RCC_CFGR_ADCPRE);
    RCC->CFGR |= RCC_CFGR_ADCPRE_DIV2; // ADC clock = RCC / 2
    // sampling time - 1.5 cycles for all chanels
    //ADC1->SMPR1 = ADC_SMPR1_SMP10_0 | ADC_SMPR1_SMP16_0 | ADC_SMPR1_SMP17_0;//channels 10, 16 and 17
    // single mode, enable vref & Tsens; wake up ADC
    ADC1->CR2 |= ADC_CR2_TSVREFE | ADC_CR2_ADON;
    // wait for Tstab - at least 1us
    while(++ctr < 0xff) nop();
    // calibration
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    ctr = 0; while((ADC1->CR2 & ADC_CR2_RSTCAL) && ++ctr < 0xfffff);
    ADC1->CR2 |= ADC_CR2_CAL;
    ctr = 0; while((ADC1->CR2 & ADC_CR2_CAL) && ++ctr < 0xfffff);
}
