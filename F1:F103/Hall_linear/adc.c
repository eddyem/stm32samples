/*
 * This file is part of the hallinear project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

uint16_t ADC_array[ADC_CHANNELS*9];

void adc_setup(){
    uint32_t ctr = 0;
    // Enable clocking
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    __DSB();
    // DMA configuration
    DMA1_Channel1->CPAR = (uint32_t) (&(ADC1->DR));
    DMA1_Channel1->CMAR = (uint32_t)(ADC_array);
    DMA1_Channel1->CNDTR = ADC_CHANNELS * 9;
    DMA1_Channel1->CCR = DMA_CCR_MINC | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0
                          | DMA_CCR_CIRC | DMA_CCR_PL | DMA_CCR_EN;
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_ADCPRE)) | RCC_CFGR_ADCPRE_DIV8; // ADC clock = RCC / 8
    // sampling time - 239.5 cycles for channels 0, 16 and 17
    ADC1->SMPR2 = ADC_SMPR2_SMP0;
    ADC1->SMPR1 = ADC_SMPR1_SMP16 | ADC_SMPR1_SMP17;
    // sequence order: 7[0] -> 16[tsen] -> 17[vdd]
    ADC1->SQR3 = (7 << 0) | (16<<5) | (17 << 10);
    ADC1->SQR1 = (ADC_CHANNELS - 1) << 20; // amount of conversions
    ADC1->CR1 = ADC_CR1_SCAN; // scan mode
    // DMA, continuous mode; enable vref & Tsens; enable SWSTART as trigger
    ADC1->CR2 = ADC_CR2_DMA | ADC_CR2_TSVREFE | ADC_CR2_CONT | ADC_CR2_EXTSEL | ADC_CR2_EXTTRIG;
    // wake up ADC
    ADC1->CR2 |= ADC_CR2_ADON;
    __DSB();
    // wait for Tstab - at least 1us
    IWDG->KR = IWDG_REFRESH;
    while(++ctr < 0xff) nop();
    // calibration
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    ctr = 0; while((ADC1->CR2 & ADC_CR2_RSTCAL) && ++ctr < 0xfffff) IWDG->KR = IWDG_REFRESH;
    ADC1->CR2 |= ADC_CR2_CAL;
    ctr = 0; while((ADC1->CR2 & ADC_CR2_CAL) && ++ctr < 0xfffff) IWDG->KR = IWDG_REFRESH;
    // clear possible errors and start
    ADC1->SR = 0;
    ADC1->CR2 |= ADC_CR2_SWSTART;
}


/**
 * @brief getADCval - calculate median value for `nch` channel
 * @param nch - number of channel
 * @return
 */
uint16_t getADCval(int nch){
    int i, addr = nch;
#define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); }
#define PIX_SWAP(a,b) { register uint16_t temp=(a);(a)=(b);(b)=temp; }
    uint16_t p[9];
    for(i = 0; i < 9; ++i, addr += ADC_CHANNELS){ // first we should prepare array for optmed
        p[i] = ADC_array[addr];
    }
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

// get voltage @input nch (1/100V)
uint32_t getADCvoltage(int nch){
    uint32_t v = getADCval(nch);
    v *= getVdd();
    v /= 0xfff; // 12bit ADC
    return v;
}

// return MCU temperature (degrees of celsius * 10)
int32_t getMCUtemp(){
    // Temp = (V25 - Vsense)/Avg_Slope + 25
    // V_25 = 1.45V,  Slope = 4.3e-3
    int32_t Vsense = getVdd() * getADCval(ADC_CH_TSEN);
    int32_t temperature = 593920 - Vsense; // 593920 == 145*4096
    temperature /= 172; // == /(4096*10*4.3e-3), 10 - to convert from *100 to *10
    temperature += 250;
    return(temperature);
}

// return Vdd * 100 (V)
uint32_t getVdd(){
    uint32_t vdd = 120 * 4096; // 1.2V
    vdd /= getADCval(ADC_CH_VDD);
    return vdd;
}
