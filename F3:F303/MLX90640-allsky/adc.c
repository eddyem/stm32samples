/*
 * This file is part of the ir-allsky project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

/**
 * @brief ADCx_array - arrays for ADC channels with median filtering:
 * ADC1:
 * 0 - Ch0 - ADC1_IN1
 * 1 - Ch1 - ADC1_IN2
 * 2 - internal Tsens - ADC1_IN16
 * 3 - Vref - ADC1_IN18
 * ADC2:
 * 4 - AIN5/DAC_OUT1 - PA4 - DAC1_OUT1 (onboard heater?), PA5 - ADC2_IN2 (DAC output control)
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
// Setup ADC and DAC
void adc_setup(){
    RCC->AHBENR |= RCC_AHBENR_ADC12EN; //  Enable clocking
    ADC12_COMMON->CCR = ADC_CCR_TSEN | ADC_CCR_VREFEN | ADC_CCR_CKMODE; // enable Tsens and Vref, HCLK/4
    calADC(ADC1);
    calADC(ADC2);
    // ADC1: channels 1,2,16,18; ADC2: channel 2
    ADC1->SMPR1 = ADC_SMPR1_SMP0 | ADC_SMPR1_SMP1;
    ADC1->SMPR2 = ADC_SMPR2_SMP15 | ADC_SMPR2_SMP17;
    // 4 conversions in group: 1->2->16->18
    ADC1->SQR1 = (1<<6) | (2<<12) | (16<<18) | (18<<24) | (NUMBER_OF_ADC1_CHANNELS-1);
    ADC2->SMPR1 = ADC_SMPR1_SMP1;
    ADC2->SQR1 = (2<<6) | (NUMBER_OF_ADC2_CHANNELS-1);
    // configure DMA for ADC
    RCC->AHBENR |= RCC_AHBENR_DMA1EN | RCC_AHBENR_DMA2EN;
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
    // enable DAC
    RCC->APB1ENR |= RCC_APB1ENR_DAC1EN;
    // DAC simple throw out constant value: output buffer disable, DAC ch1 enable
    DAC->CR = DAC_CR_BOFF1 | DAC_CR_EN1;
    // starting value: 0
    DAC1->DHR12R1 = 0;
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
    int adval = (nch >= NUMBER_OF_ADC1_CHANNELS) ? NUMBER_OF_ADC2_CHANNELS : NUMBER_OF_ADC1_CHANNELS;
    int addr = (nch >= NUMBER_OF_ADC1_CHANNELS) ? nch - NUMBER_OF_ADC2_CHANNELS + ADC2START: nch;
    for(int i = 0; i < 9; ++i, addr += adval) // first we should prepare array for optmed
        p[i] = ADC_array[addr];
    PIX_SORT(p[1], p[2]) ; PIX_SORT(p[4], p[5]) ; PIX_SORT(p[7], p[8]) ;
    PIX_SORT(p[0], p[1]) ; PIX_SORT(p[3], p[4]) ; PIX_SORT(p[6], p[7]) ;
    PIX_SORT(p[1], p[2]) ; PIX_SORT(p[4], p[5]) ; PIX_SORT(p[7], p[8]) ;
    PIX_SORT(p[0], p[3]) ; PIX_SORT(p[5], p[8]) ; PIX_SORT(p[4], p[7]) ;
    PIX_SORT(p[3], p[6]) ; PIX_SORT(p[1], p[4]) ; PIX_SORT(p[2], p[5]) ;
    PIX_SORT(p[4], p[7]) ; PIX_SORT(p[4], p[2]) ; PIX_SORT(p[6], p[4]) ;
    PIX_SORT(p[4], p[2]) ;
    return p[4];
#undef PIX_SORT
#undef PIX_SWAP
}

// get voltage @input nch (V)
float getADCvoltage(int nch){
    float v = getADCval(nch);
    v *= getVdd();
    v /= 4096.f; // 12bit ADC
    return v;
}

// return MCU temperature (degrees of celsius)
float getMCUtemp(){
    // make correction on Vdd value
    int32_t ADval = getADCval(ADC_TS);
    float temperature = (float) *TEMP30_CAL_ADDR - ADval;
    temperature *= (110.f - 30.f);
    temperature /= (float)(*TEMP30_CAL_ADDR - *TEMP110_CAL_ADDR);
    temperature += 30.f;
    return(temperature);
}

// return Vdd (V)
float getVdd(){
    float vdd = ((float) *VREFINT_CAL_ADDR) * 3.3f; // 3.3V
    vdd /= getADCval(ADC_VREF);
    return vdd;
}
