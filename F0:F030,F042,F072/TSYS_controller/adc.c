/*
 * This file is part of the TSYS_controller project.
 * Copyright 2019 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
 * 0..3 - external channels
 * 4 - internal Tsens
 * 5 - Vref
 */
#define TSENS_CHAN  (NUMBER_OF_ADC_CHANNELS-2)
#define VREF_CHAN   (NUMBER_OF_ADC_CHANNELS-1)
static uint16_t ADC_array[NUMBER_OF_ADC_CHANNELS*9];

/*
 * ADC channels:
 * IN0 - V12
 * IN1 - V5
 * IN3 - I12
 * IN6 - V3.3
 * IN16- temperature sensor
 * IN17- vref
 */
void adc_setup(){
    uint16_t ctr = 0; // 0xfff0 - more than 1.3ms
    // Enable clocking
    /* (1) Enable the peripheral clock of the ADC */
    /* (2) Start HSI14 RC oscillator */
    /* (3) Wait HSI14 is ready */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; /* (1) */
    RCC->CR2 |= RCC_CR2_HSI14ON; /* (2) */
    while ((RCC->CR2 & RCC_CR2_HSI14RDY) == 0 && ++ctr < 0xfff0){}; /* (3) */
    // calibration
    /* (1) Ensure that ADEN = 0 */
    /* (2) Clear ADEN */
    /* (3) Launch the calibration by setting ADCAL */
    /* (4) Wait until ADCAL=0 */
    if ((ADC1->CR & ADC_CR_ADEN) != 0){ /* (1) */
        ADC1->CR &= (uint32_t)(~ADC_CR_ADEN);  /* (2) */
    }
    ADC1->CR |= ADC_CR_ADCAL; /* (3) */
    ctr = 0; // ADC calibration time is 5.9us
    while ((ADC1->CR & ADC_CR_ADCAL) != 0 && ++ctr < 0xfff0){}; /* (4) */
    // enable ADC
    ctr = 0;
    do{
          ADC1->CR |= ADC_CR_ADEN;
    }while ((ADC1->ISR & ADC_ISR_ADRDY) == 0 && ++ctr < 0xfff0);
    // configure ADC
    /* (1) Select HSI14 by writing 00 in CKMODE (reset value) */
    /* (2) Select the continuous mode */
    /* (3) Select CHSEL0,1,3,6 - ADC inputs, 16,17 - t. sensor and vref */
    /* (4) Select a sampling mode of 111 i.e. 239.5 ADC clk to be greater than 17.1us */
    /* (5) Wake-up the VREFINT and Temperature sensor (only for VBAT, Temp sensor and VRefInt) */
    // ADC1->CFGR2 &= ~ADC_CFGR2_CKMODE; /* (1) */
    ADC1->CFGR1 |= ADC_CFGR1_CONT; /* (2)*/
    ADC1->CHSELR = ADC_CHSELR_CHSEL0 | ADC_CHSELR_CHSEL1 | ADC_CHSELR_CHSEL3 |
            ADC_CHSELR_CHSEL6 | ADC_CHSELR_CHSEL16 | ADC_CHSELR_CHSEL17; /* (3)*/
    ADC1->SMPR |= ADC_SMPR_SMP_0 | ADC_SMPR_SMP_1 | ADC_SMPR_SMP_2; /* (4) */
    ADC->CCR |= ADC_CCR_TSEN | ADC_CCR_VREFEN; /* (5) */
    // configure DMA for ADC
    // DMA for AIN
    /* (1) Enable the peripheral clock on DMA */
    /* (2) Enable DMA transfer on ADC and circular mode */
    /* (3) Configure the peripheral data register address */
    /* (4) Configure the memory address */
    /* (5) Configure the number of DMA tranfer to be performs on DMA channel 1 */
    /* (6) Configure increment, size, interrupts and circular mode */
    /* (7) Enable DMA Channel 1 */
    RCC->AHBENR |= RCC_AHBENR_DMA1EN; /* (1) */
    ADC1->CFGR1 |= ADC_CFGR1_DMAEN | ADC_CFGR1_DMACFG; /* (2) */
    DMA1_Channel1->CPAR = (uint32_t) (&(ADC1->DR)); /* (3) */
    DMA1_Channel1->CMAR = (uint32_t)(ADC_array); /* (4) */
    DMA1_Channel1->CNDTR = NUMBER_OF_ADC_CHANNELS * 9; /* (5) */
    DMA1_Channel1->CCR |= DMA_CCR_MINC | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_CIRC; /* (6) */
    DMA1_Channel1->CCR |= DMA_CCR_EN; /* (7) */
    ADC1->CR |= ADC_CR_ADSTART; /* start the ADC conversions */
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
    getVdd();
    // make correction on Vdd value
//    int32_t temperature = (int32_t)ADC_array[4] * VddValue / 330;
    int32_t ADval = getADCval(TSENS_CHAN);
    int32_t temperature = (int32_t) *TEMP30_CAL_ADDR - ADval;
    temperature *= (int32_t)(1100 - 300);
    temperature /= (int32_t)(*TEMP30_CAL_ADDR - *TEMP110_CAL_ADDR);
    temperature += 300;
    return(temperature);
}

// return Vdd * 100 (V)
uint32_t getVdd(){
    uint32_t vdd = ((uint32_t) *VREFINT_CAL_ADDR) * (uint32_t)330; // 3.3V
    vdd /= getADCval(VREF_CHAN);
    return vdd;
}

static inline uint32_t Ufromadu(uint8_t nch, uint32_t vdd){
    uint32_t ADU = getADCval(nch);
    ADU *= vdd;
    ADU >>= 12; // /4096
    return ADU;
}

/**
 * @brief getUval - calculate U & I
 * @return array with members:
 *      0 - V12 * 100V (U12 = 12Vin/4.93)
 *      1 - V5  * 100V (U5  = 5Vin /2)
 *      2 - I12 mA     (U = 1V/1A)
 *      3 - V3.3* 100V (U3.3= 3.3Vin/2)
 */
uint16_t *getUval(){
    static uint16_t Uval[4];
    uint32_t vdd = getVdd();
    uint32_t val = Ufromadu(0, vdd) * 493;
    Uval[0] = (uint16_t)(val / 100);
    Uval[1] = (uint16_t)(Ufromadu(1, vdd) << 1);
    val = getADCval(2) * vdd * 10;
    Uval[2] = (uint16_t)(val >> 12);
    Uval[3] = (uint16_t)(Ufromadu(3, vdd) << 1);
    return Uval;
}
