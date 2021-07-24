/*
 * This file is part of the canrelay project.
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

#include "adc.h"

/**
 * @brief ADC_array - array for ADC channels with median filtering:
 * 0 & 1 - external channels (5 & 12V)
 * 2 - internal Tsens
 * 3 - Vref
 */
static uint16_t ADC_array[NUMBER_OF_ADC_CHANNELS*9];

void adc_setup(){
    uint16_t ctr = 0; // 0xfff0 - more than 1.3ms
    ADC1->CR &= ~ADC_CR_ADEN;
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; // Enable the peripheral clock of the ADC
    RCC->CR2 |= RCC_CR2_HSI14ON; // Start HSI14 RC oscillator
    while ((RCC->CR2 & RCC_CR2_HSI14RDY) == 0 && ++ctr < 0xfff0){}; // Wait HSI14 is ready
    // calibration
    if(ADC1->CR & ADC_CR_ADEN){ // Ensure that ADEN = 0
        ADC1->CR &= (uint32_t)(~ADC_CR_ADEN);  // Clear ADEN
    }
    ADC1->CR |= ADC_CR_ADCAL; // Launch the calibration by setting ADCAL
    ctr = 0; // ADC calibration time is 5.9us
    while(ADC1->CR & ADC_CR_ADCAL && ++ctr < 0xfff0); // Wait until ADCAL=0
    // enable ADC
    ctr = 0;
    do{
          ADC1->CR |= ADC_CR_ADEN;
    }while((ADC1->ISR & ADC_ISR_ADRDY) == 0 && ++ctr < 0xfff0);
    // configure ADC
    ADC1->CFGR1 |= ADC_CFGR1_CONT; // Select the continuous mode
    // channels 6,7,16 and 17
    ADC1->CHSELR =  ADC_CHSELR_CHSEL6 | ADC_CHSELR_CHSEL7 | ADC_CHSELR_CHSEL16 | ADC_CHSELR_CHSEL17;
    ADC1->SMPR |= ADC_SMPR_SMP; // Select a sampling mode of 111 i.e. 239.5 ADC clk to be greater than 17.1us
    ADC->CCR |= ADC_CCR_TSEN | ADC_CCR_VREFEN; // Wake-up the VREFINT and Temperature sensor
    // configure DMA for ADC
    RCC->AHBENR |= RCC_AHBENR_DMA1EN; // Enable the peripheral clock on DMA
    ADC1->CFGR1 |= ADC_CFGR1_DMAEN | ADC_CFGR1_DMACFG; // Enable DMA transfer on ADC and circular mode
    DMA1_Channel1->CPAR = (uint32_t) (&(ADC1->DR)); // Configure the peripheral data register address
    DMA1_Channel1->CMAR = (uint32_t)(ADC_array); // Configure the memory address
    DMA1_Channel1->CNDTR = NUMBER_OF_ADC_CHANNELS * 9; // Configure the number of DMA tranfer to be performs on DMA channel 1
    DMA1_Channel1->CCR |= DMA_CCR_MINC | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_CIRC; // Configure increment, size, interrupts and circular mode
    DMA1_Channel1->CCR |= DMA_CCR_EN; // Enable DMA Channel 1
    ADC1->CR |= ADC_CR_ADSTART; // start the ADC conversions
}

/**
 * @brief getADCval - calculate median value for `nch` channel
 * @param nch - number of channel
 * @return
 */
uint16_t getADCval(int nch){
    int i, addr = nch;
    register uint16_t temp;
#define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); }
#define PIX_SWAP(a,b) { temp=(a);(a)=(b);(b)=temp; }
    uint16_t p[9];
    for(i = 0; i < 9; ++i, addr += NUMBER_OF_ADC_CHANNELS) // first we should prepare array for optmed
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

// return MCU temperature (degrees of celsius * 10)
int32_t getMCUtemp(){
    int32_t ADval = getADCval(2);
    int32_t temperature = (int32_t) *TEMP30_CAL_ADDR - ADval;
    temperature *= (int32_t)(1100 - 300);
    temperature /= (int32_t)(*TEMP30_CAL_ADDR - *TEMP110_CAL_ADDR);
    temperature += 300;
    return(temperature);
}

// return Vdd * 100 (V)
uint32_t getVdd(){
    uint32_t vdd = ((uint32_t) *VREFINT_CAL_ADDR) * (uint32_t)330; // 3.3V
    vdd /= getADCval(3);
    return vdd;
}
