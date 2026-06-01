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

#include <math.h>

#ifdef EBUG
#include "strfunc.h"
#endif

#include "adc.h"

/**
 * @brief ADCx_array - arrays for ADC channels with median filtering:
 * ADC1:
 * 0 - Ch0 - ADC1_IN1 - NTC1
 * 1 - Ch1 - ADC1_IN2 - NTC2
 * 2 - Ch2 - ADC1_IN3 - NTC3
 * 3 - Ch3 - ADC1_IN4 - NTC4
 * 4 - internal Tsens - ADC1_IN16
 * 5 - Vref - ADC1_IN18
 * ADC2:
 *  AIN5/DAC_OUT1 - PA4 - DAC1_OUT1 (onboard heater)
 * 6 - PA5 - ADC2_IN2 (DAC output control)
 */
static uint16_t ADC_array[NUMBER_OF_ADC_CHANNELS*9];

TRUE_INLINE void calADC(ADC_TypeDef *chnl){
    // calibration
    // enable voltage regulator
    chnl->CR = 0;
    chnl->CR = ADC_CR_ADVREGEN_0;
    // wait for 10us
    uint16_t ctr = 0;
    while(++ctr < 1000){IWDG->KR = IWDG_REFRESH;}
    // ADCALDIF=0 (single channels)
    if((chnl->CR & ADC_CR_ADEN)){
        chnl->CR |= ADC_CR_ADSTP;
        chnl->CR |= ADC_CR_ADDIS;
    }
    chnl->CR |= ADC_CR_ADCAL;
    while((chnl->CR & ADC_CR_ADCAL) != 0 && ++ctr < 0xfff0){IWDG->KR = IWDG_REFRESH;};
    chnl->CR = ADC_CR_ADVREGEN_0;
    // enable ADC
    ctr = 0;
    do{
        chnl->CR |= ADC_CR_ADEN;
        IWDG->KR = IWDG_REFRESH;
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
// Setup ADC and DAC; ADC/DAC pins should be prepared in gpio_setup
void adc_setup(){
    RCC->AHBENR |= RCC_AHBENR_ADC12EN; //  Enable clocking
    ADC12_COMMON->CCR = ADC_CCR_TSEN | ADC_CCR_VREFEN | ADC_CCR_CKMODE; // enable Tsens and Vref, HCLK/4
    calADC(ADC1);
    calADC(ADC2);
    // ADC1: channels 1,2,3,4,16,18; 601.5 clock cycles
    //ADC1->SMPR1 = ADC_SMPR1_SMP0 | ADC_SMPR1_SMP1 | ADC_SMPR1_SMP2 | ADC_SMPR1_SMP3;
    ADC1->SMPR1 = ADC_SMPR1_SMP1 | ADC_SMPR1_SMP2 | ADC_SMPR1_SMP3 | ADC_SMPR1_SMP4;
    //ADC1->SMPR2 = ADC_SMPR2_SMP15 | ADC_SMPR2_SMP17;
    ADC1->SMPR2 = ADC_SMPR2_SMP16 | ADC_SMPR2_SMP18;
    // 6 conversions in group: 1->2->3->4->16->18
    ADC1->SQR1 = (1<<6) | (2<<12) | (3<<18) | (4<<24) | (NUMBER_OF_ADC1_CHANNELS-1);
    ADC1->SQR2 = (16<<0) | (18<<6);
    // ADC2: channel 2
    ADC2->SMPR1 = ADC_SMPR1_SMP2;
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
uint16_t getADCval(uint8_t nch){
    if(nch >= NUMBER_OF_ADC_CHANNELS) return 0;
    uint16_t temp;
#define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); }
#define PIX_SWAP(a,b) { temp=(a);(a)=(b);(b)=temp; }
    uint16_t p[9];
    int adval = (nch >= NUMBER_OF_ADC1_CHANNELS) ? NUMBER_OF_ADC2_CHANNELS : NUMBER_OF_ADC1_CHANNELS;
    int addr = (nch >= NUMBER_OF_ADC1_CHANNELS) ? nch - NUMBER_OF_ADC1_CHANNELS + ADC2START: nch;
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
float getADCvoltage(uint8_t nch){
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

// R lookup table for T=-10..59 degreesC
#if 0
T=[-10:59]+273.15;
R=1000*exp(3950*(1./T-1/298.15));
for i=1:length(T); printf("\t%.1f,\t// %d \n", R(i), T(i)-273.15); endfor
#endif

static const float Rlut[] = {
    5824.6, // -10
    5502.8, // -9
    5201.1, // -8
    4917.9, // -7
    4652.2, // -6
    4402.6, // -5
    4168.1, // -4
    3947.7, // -3
    3740.5, // -2
    3545.5, // -1
    3362.1, // 0
    3189.3, // 1
    3026.6, // 2
    2873.3, // 3
    2728.8, // 4
    2592.5, // 5
    2463.9, // 6
    2342.5, // 7
    2227.9, // 8
    2119.7, // 9
    2017.5, // 10
    1920.8, // 11
    1829.4, // 12
    1743.0, // 13
    1661.2, // 14
    1583.7, // 15
    1510.4, // 16
    1440.9, // 17
    1375.1, // 18
    1312.7, // 19
    1253.5, // 20
    1197.4, // 21
    1144.1, // 22
    1093.6, // 23
    1045.6, // 24
    1000.0, // 25
    956.7,  // 26
    915.5,  // 27
    876.4,  // 28
    839.1,  // 29
    803.7,  // 30
    770.0,  // 31
    737.9,  // 32
    707.4,  // 33
    678.3,  // 34
    650.6,  // 35
    624.1,  // 36
    598.9,  // 37
    574.9,  // 38
    552.0,  // 39
    530.1,  // 40
    509.3,  // 41
    489.4,  // 42
    470.3,  // 43
    452.2,  // 44
    434.8,  // 45
    418.2,  // 46
    402.4,  // 47
    387.2,  // 48
    372.7,  // 49
    358.8,  // 50
    345.5,  // 51
    332.8,  // 52
    320.7,  // 53
    309.0,  // 54
    297.8,  // 55
    287.1,  // 56
    276.9,  // 57
    267.1,  // 58
    257.7,  // 59
};

#define LUTSZ   (sizeof(Rlut) / sizeof(float))

/**
 * @brief getNTCtemp - stupid LUT-search and linear approximation of T by R
 * @param nch - channel of ADC for Tx
 * @return temperature in degr.C
 */
float getNTCtemp(uint8_t nch){
    if(nch > ADC_AIN4) return -300.f; // bad number
    uint16_t val = getADCval(nch);
    if(val < 5) return -400.f; // short cirquit
    else if(val > 4090) return -500.f; // no NTC
    float R = 1000.f / (4096.f / val - 1.f); // resistance of NTC
#ifdef EBUG
    USB_sendstr("R="); USB_sendstr(float2str(R, 1)); newline();
#endif
    int left = 0, right = LUTSZ-1;
    if(R > Rlut[0]) right = 1;
    else if(R < Rlut[LUTSZ-1]) left = LUTSZ-2;
    while(right - left > 1){
        int idx = left + (right - left) / 2;
        float Rl = Rlut[idx];
        if(Rl > R) left = idx + 1;
        else right = idx - 1;
    }
    if(left >= (int)LUTSZ) return 60.f;
    float Rleft = Rlut[left], Rright = Rlut[left+1];
    float T = (float)left - 9.f - (R - Rright) / (Rleft - Rright);
    return T;
}
