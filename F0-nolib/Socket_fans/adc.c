/*
 * This file is part of the Chiller project.
 * Copyright 2018 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
 * 0..3 - external NTC
 * 4 - ext 12V
 * 5 - ext 5V
 * 6 - internal Tsens
 * 7 - Vref
 */
uint16_t ADC_array[NUMBER_OF_ADC_CHANNELS*9];

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
//    getVdd();
    // make correction on Vdd value
//    int32_t temperature = (int32_t)ADC_array[4] * VddValue / 330;
    int32_t ADval = getADCval(6);
    int32_t temperature = (int32_t) *TEMP30_CAL_ADDR - ADval;
    temperature *= (int32_t)(1100 - 300);
    temperature /= (int32_t)(*TEMP30_CAL_ADDR - *TEMP110_CAL_ADDR);
    temperature += 300;
    return(temperature);
}

// return Vdd * 100 (V)
uint32_t getVdd(){
    uint32_t vdd = ((uint32_t) *VREFINT_CAL_ADDR) * (uint32_t)330; // 3.3V
    vdd /= getADCval(7);
    return vdd;
}

// convert ADU into Volts (*100)
static inline uint32_t Ufromadu(uint8_t nch){
    uint32_t ADU = getADCval(nch);
    ADU *= getVdd();
    ADU >>= 12; // /4096
    return ADU;
}

// get 12V (*100)
uint32_t getU12(){
    uint32_t V = Ufromadu(4) * 493;
    return V / 100;
}

// get 5V (*100)
uint32_t getU5(){
    return Ufromadu(5) * 2;
}

/**
 * @brief getNTC - return temperature of NTC (*10 degrC)
 * @param nch - NTC channel number (0..3)
 * @return
 */
int16_t getNTC(int nch){
#define NKNOTS  (14)
    const int16_t ADU[NKNOTS] = {732, 945, 1197, 2035, 2309, 2542, 2739, 2859, 2969, 3068, 3154, 3228, 3293, 3347};
    const int16_t T[NKNOTS]   = {100, 160, 227, 438, 508, 571, 628, 666, 702, 738, 772, 805, 837, 869};
    /*
     * coefficients: 0.028261   0.026536   0.025136   0.025738   0.027044   0.028922   0.031038   0.033255   0.036056   0.039606   0.044173   0.050296   0.058920   0.071729
     * use
     * [N D] = rat(K*10); printf("N="); printf("%d, ", N); printf("\nD="); printf("%d, ", D);printf("\n");
     */
    const int16_t N[NKNOTS] = {13, 95, 185, 96, 43, 153, 347, 141, 128, 261, 235, 85, 393, 307};
    const int16_t D[NKNOTS] = {46, 358, 736, 373, 159, 529, 1118, 424, 355, 659, 532, 169, 667, 428};

    if(nch < 0 || nch > 3) return -30000;
    uint16_t val = getADCval(nch);
    // find interval
    int idx = (NKNOTS+1)/2; // middle
    while(idx > 0 && idx < NKNOTS){
        int16_t left = ADU[idx];
        int half = idx / 2;
        if(val < left){
            if(idx == 0) break;
            if(val > ADU[idx-1]){ // found
                --idx;
                break;
            }
            idx = half;
        }else{
            if(idx == NKNOTS - 1) break; // more than max value
            if(val < ADU[idx+1]) break;  // found
            idx += half;
        }
    }
    if(idx < 0) idx = 0;
    else if(idx > NKNOTS-1) idx = NKNOTS - 1;
    // T = Y0(idx) + K(idx) * (ADU - X0(idx));
    int16_t valT = T[idx] + (N[idx]*(val - ADU[idx]))/D[idx];
#undef NKNOTS
    return valT;
}
