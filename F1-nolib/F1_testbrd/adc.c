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
 * 0 - Rvar
 * 1 - internal Tsens
 * 2 - Vref
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
    // Temp = (V25 - Vsense)/Avg_Slope + 25
    // V_25 = 1.45V,  Slope = 4.3e-3
    int32_t Vsense = getVdd() * getADCval(1);
    Vsense /= 4096;
    int32_t temperature = 145 - Vsense;
    temperature *= 233;
    temperature /= 10; // convert from *100 to *10
    temperature += 250;
    return(temperature);
}

// return Vdd * 100 (V)
uint32_t getVdd(){
    uint32_t vdd = ((uint32_t) *VREFINT_CAL_ADDR) * (uint32_t)300; // 3.0V
    vdd /= getADCval(2);
    return vdd;
}
