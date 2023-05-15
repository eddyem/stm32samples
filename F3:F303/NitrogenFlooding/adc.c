/*
 * This file is part of the nitrogen project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "adc.h"
#include "hardware.h" // ADCvals

/**
 * @brief ADCx_array - arrays for ADC channels with median filtering:
 * ADC1:
 * 0..9 - AIN0..9 (ADC1_IN1..10)
 * 10 - internal Tsens - ADC1_IN16
 * ADC2:
 * 11 - AINext - (ADC2 in 1)
 */
static uint16_t ADC_array[NUMBER_OF_ADC_CHANNELS*9];

TRUE_INLINE void calADC(ADC_TypeDef *chnl){
    // calibration
    // enable voltage regulator
    chnl->CR = 0;
    chnl->CR = ADC_CR_ADVREGEN_0;
    // wait for 10us
    uint16_t ctr = 0;
    while(++ctr < 1000){nop();}
    // ADCALDIF=0 (single channels)
    if((chnl->CR & ADC_CR_ADEN)){
        chnl->CR |= ADC_CR_ADSTP;
        chnl->CR |= ADC_CR_ADDIS;
    }
    chnl->CR |= ADC_CR_ADCAL;
    while((chnl->CR & ADC_CR_ADCAL) != 0 && ++ctr < 0xfff0){};
    chnl->CR = ADC_CR_ADVREGEN_0;
    // enable ADC
    ctr = 0;
    do{
          chnl->CR |= ADC_CR_ADEN;
    }while((chnl->ISR & ADC_ISR_ADRDY) == 0 && ++ctr < 0xfff0);
}

TRUE_INLINE void enADC(ADC_TypeDef *chnl){
    // ADEN->1, wait ADRDY
    chnl->CR |= ADC_CR_ADEN;
    uint16_t ctr = 0;
    while(!(chnl->ISR & ADC_ISR_ADRDY) && ++ctr < 0xffff){}
    chnl->CR |= ADC_CR_ADSTART; /* start the ADC conversions */
}

/**
 * ADC1 - DMA1_ch1
 * ADC2 - DMA2_ch1
 */
// Setup ADC
void adc_setup(){
    IWDG->KR = IWDG_REFRESH;
    RCC->AHBENR |= RCC_AHBENR_ADC12EN; //  Enable clocking
    ADC12_COMMON->CCR = ADC_CCR_TSEN | ADC_CCR_CKMODE; // enable Tsens, HCLK/4
    calADC(ADC1);
    calADC(ADC2);
    // ADC1: channels 1-10,16; ADC2: channel 1
    ADC1->SMPR1 = ADC_SMPR1_SMP1 | ADC_SMPR1_SMP2 | ADC_SMPR1_SMP3 | ADC_SMPR1_SMP4 | ADC_SMPR1_SMP5
                | ADC_SMPR1_SMP6 | ADC_SMPR1_SMP7 | ADC_SMPR1_SMP8 | ADC_SMPR1_SMP9 ;
    ADC1->SMPR2 = ADC_SMPR2_SMP10 | ADC_SMPR2_SMP16;
    // 11 conversions in group: 1...10->16
    ADC1->SQR1 = (1<<6) | (2<<12) | (3<<18) | (4<<24) | (NUMBER_OF_ADC1_CHANNELS-1);
    ADC1->SQR2 = (5<<0) | (6<<6) | (7<<12) | (8<<18) | (9<<24);
    ADC1->SQR3 = (10<<0)| (16<<6);
    ADC2->SMPR1 = ADC_SMPR1_SMP1;
    ADC2->SQR1 = (1<<6) | (NUMBER_OF_ADC2_CHANNELS-1);
    // configure DMA for ADC
    ADC1->CFGR = ADC_CFGR_CONT | ADC_CFGR_DMAEN | ADC_CFGR_DMACFG;
    ADC2->CFGR = ADC_CFGR_CONT | ADC_CFGR_DMAEN | ADC_CFGR_DMACFG;
    DMA1_Channel1->CPAR = (uint32_t) (&(ADC1->DR));
    DMA1_Channel1->CMAR = (uint32_t)(ADC_array);
    DMA1_Channel1->CNDTR = NUMBER_OF_ADC1_CHANNELS * 9;
    DMA1_Channel1->CCR |= DMA_CCR_MINC | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_CIRC;
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    DMA2_Channel1->CPAR = (uint32_t) (&(ADC2->DR));
    DMA2_Channel1->CMAR = (uint32_t)(&ADC_array[ADC2START]);
    DMA2_Channel1->CNDTR = NUMBER_OF_ADC2_CHANNELS * 9;
    DMA2_Channel1->CCR |= DMA_CCR_MINC | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_CIRC;
    DMA2_Channel1->CCR |= DMA_CCR_EN;

    enADC(ADC1);
    enADC(ADC2);
}

/**
 * @brief getADCval - calculate median value for `nch` channel
 * @param nch - number of channel
 * @return
 */
uint16_t getADCval(int nch){
    register uint16_t temp;
#define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); }
#define PIX_SWAP(a,b) { temp=(a);(a)=(b);(b)=temp; }
    uint16_t p[9];
    int addr = nch, adval = NUMBER_OF_ADC1_CHANNELS;
    if(nch >= NUMBER_OF_ADC1_CHANNELS){
        adval = NUMBER_OF_ADC2_CHANNELS;
        addr += ADC2START - NUMBER_OF_ADC1_CHANNELS;
    }
    for(int i = 0; i < 9; ++i, addr += adval) // first we should prepare array for optmed
        p[i] = ADC_array[addr];
    PIX_SORT(p[1], p[2]) ; PIX_SORT(p[4], p[5]) ; PIX_SORT(p[7], p[8]) ;
    PIX_SORT(p[0], p[1]) ; PIX_SORT(p[3], p[4]) ; PIX_SORT(p[6], p[7]) ;
    PIX_SORT(p[1], p[2]) ; PIX_SORT(p[4], p[5]) ; PIX_SORT(p[7], p[8]) ;
    PIX_SORT(p[0], p[3]) ; PIX_SORT(p[5], p[8]) ; PIX_SORT(p[4], p[7]) ;
    PIX_SORT(p[3], p[6]) ; PIX_SORT(p[1], p[4]) ; PIX_SORT(p[2], p[5]) ;
    PIX_SORT(p[4], p[7]) ; PIX_SORT(p[4], p[2]) ; PIX_SORT(p[6], p[4]) ;
    PIX_SORT(p[4], p[2]) ;
#undef PIX_SORT
#undef PIX_SWAP
    return p[4];
}

// get voltage @input nch (V)
float getADCvoltage(uint16_t ADCval){
    float v = (float)ADCval * 3.3;
    return v/4096.f; // 12bit ADC
}

// return MCU temperature (degrees of celsius)
float getMCUtemp(){
    float temperature = ADCvals[ADC_TSENS] - (float) *TEMP30_CAL_ADDR;
    temperature *= (110.f - 30.f);
    temperature /= (float)(*TEMP110_CAL_ADDR - *TEMP30_CAL_ADDR);
    temperature += 30.f;
    return(temperature);
}

// calculate R (Ohms) by given `ADCval` for main 10 ADC channels with 1k in upper arm of divider
float calcR(uint16_t ADCval){
    return 1000.f/(4096.f/((float)ADCval) - 1.f);
}

/****** R(T, K):
T -= 273.15; % convert to K
_A = 3.9083e-03;
_B = -5.7750e-07;
_C = 0.;
if(T < 0.); _C = -4.1830e-12; endif
R = 1000.*(1 + _A*T + _B*T.^2 - _C.*T.^3*100. + _C.*T.^4);

=====> for T=[70:400] Kelvins

function T = pt1000Tapp(R)
	k1 = 27.645;
	k2 = 0.235268;
	k3 = 1.0242e-05;
	k4 = 0.;
	if(R < 1000)
		k1 = 31.067;
		k2 = 2.2272e-01;
		k3 = 2.5251e-05;
		k4 = -5.9001e-09;
	endif
	T = k1 + k2*R + k3*R.^2 + k4*R.^3;
endfunction

mean(T-Tapp)= -3.3824e-04
std(T-Tapp')= 3.2089e-03
max(abs(T-Tapp'))= 0.011899

********/

// approximate calculation of T (K) for platinum 1k PTC
float calcT(uint16_t ADCval){
    float R = calcR(ADCval);
    if(R < 1000.){
        return (31.067 + R * (2.2272e-01 + R * (2.5251e-05 - R * 5.9001e-09)));
    }
    return (27.645 + R * (0.235268 + R * 1.0242e-05));
}

// MPX5050: V=VS(P x 0.018 + 0.04); for 3v3 ADU=4096(P*0.018+0.04) ====>
// 0.018P=ADU/4096-0.04,
// P(kPa) = 55.556*(ADU/4096-0.04)
float calcPres5050(){
    float adu = (float)ADCvals[ADC_EXT]/4096. - 0.04;
    return 55.556*adu;
}
