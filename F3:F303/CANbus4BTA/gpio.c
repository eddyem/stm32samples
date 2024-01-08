/*
 * This file is part of the canbus4bta project.
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

#include "flash.h"
#include "gpio.h"
#include "hardware.h"

// GPIO IN management by two 74HC4051

static uint8_t eswitches[2] = {0};
static uint8_t stage = 0;
static uint32_t lastT[8][2] = {0};  // last time of ESW changed

uint8_t getESW(uint8_t nch){
    return eswitches[nch];
}

// update `eswitches`, anti-bounce
TRUE_INLINE void updesw(uint8_t nch, uint8_t newval){
    uint8_t mask = eswitches[nch] ^ newval, newmask = 0;
    if(mask == 0) return;
    for(uint8_t bit = 0; bit < 8; ++bit){ // check all bits of mask
        newmask >>= 1;
        if((mask & 1) && Tms - lastT[bit][nch] >= the_conf.bounce_ms){ // OK, change this value
            newmask |= 0x80;
            lastT[bit][nch] = Tms;
        }
        mask >>= 1;
    }
    if(newmask == 0) return;
    // now change only allowable bits
    eswitches[nch] = (eswitches[nch] & ~newmask) | (newval & newmask);
}

/**
 * @brief ESW_process - process multiplexer 1
 */
void ESW_process(){
    static uint8_t curch = 0;
    static int curaddr = 7;
    static uint8_t ESW = 0;
    switch(stage){
        case 0: // stage 0: change address and turn on multiplexer
            stage = 1;
            MULADDR_set(curaddr);
            MUL_ON(curch);
        break;
        case 1: // stage 1: read data and turn off mul
        default:
            stage = 0;
            ESW <<= 1;
            ESW |= ESW_GET();
            MUL_OFF(curch);
            if(--curaddr < 0){
                updesw(curch, ESW);
                curch = !curch;
                curaddr = 7;
            }
        break;
    }
}

static void NeverDoThisFuckingShit(){
    uint32_t ctr = 3;
    while(ctr--) nop();
}

/**
 * @brief getSwitches - blocking read of all switches of given channel
 * @param chno - number of multiplexer: 0 (board switches) or 1 (external ESW)
 */
uint8_t getSwitches(uint8_t chno){
    if(chno > 1) return 0;
    MUL_OFF(0); MUL_OFF(1);
    uint8_t state = 0;
    for(int i = 7; i > -1; --i){
        MULADDR_set(i);
        MUL_ON(chno);
        state <<= 1;
        NeverDoThisFuckingShit();
        state |= ESW_GET();
        MUL_OFF(chno);
        NeverDoThisFuckingShit();
    }
    stage = 0; // prevent interference with `ESW_process()`
    return state;
}
