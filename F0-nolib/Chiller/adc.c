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

extern volatile uint32_t Tms; // time counter for 1-second Vdd measurement
static uint32_t lastVddtime = 0; // Tms value of last Vdd measurement
static uint32_t VddValue = 0; // value of Vdd * 100 (for more precision measurements)
// check time of last Vdd measurement & refresh it value
#define CHKVDDTIME() do{if(!VddValue || Tms < lastVddtime || Tms - lastVddtime > 999) getVdd();}while(0)

/**
 * @brief ADC_array - array for ADC channels:
 * 0..3 - external NTC
 * 4 - internal Tsens
 * 5 - Vref
 */
uint16_t ADC_array[NUMBER_OF_ADC_CHANNELS];


// return MCU temperature (degrees of celsius * 10)
int32_t getMCUtemp(){
    getVdd();
    // make correction on Vdd value
    int32_t temperature = (int32_t)ADC_array[4] * VddValue / 330;
    temperature = (int32_t) *TEMP30_CAL_ADDR - temperature;
    temperature *= (int32_t)(1100 - 300);
    temperature = temperature / (int32_t)(*TEMP30_CAL_ADDR - *TEMP110_CAL_ADDR);
    temperature += 300;
    return(temperature);
}

// return Vdd * 100 (V)
uint32_t getVdd(){
/*    #define ARRSZ (10)
    static uint16_t arr[ARRSZ] = {0};
    static int arridx = 0;
    uint32_t v = ADC_array[5];
    int i;
    if(arr[0] == 0){ // first run - fill all with current data
        for(i = 0; i < ARRSZ; ++i) arr[i] = (uint16_t) v;
    }else{
        arr[arridx++] = v;
        v = 0; // now v is mean
        if(arridx > ARRSZ-1) arridx = 0;
        // calculate mean
        for(i = 0; i < ARRSZ; ++i){
            v += arr[i];
        }
        v /= ARRSZ;
    }*/
    uint32_t vdd = ((uint32_t) *VREFINT_CAL_ADDR) * (uint32_t)330; // 3.3V
    //vdd /= v;
    vdd /= ADC_array[5];
    lastVddtime = Tms;
    VddValue = vdd;
    return vdd;
}

