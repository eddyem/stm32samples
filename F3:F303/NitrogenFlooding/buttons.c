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

#include "buttons.h"
#include "hardware.h"

typedef struct{
    keyevent event;         // current key event
    int16_t counter;        // press/release counter
    uint32_t lastTms;       // time of last event change
} keybase;

static keybase allkeys[BTNSNO] = {0}; // array for buttons' states

uint32_t lastUnsleep = 0; // last keys activity time

void process_keys(){
    static uint32_t lastT = 0;
    if(Tms == lastT) return;
    uint16_t d = (uint16_t)(Tms - lastT);
    lastT = Tms;
    for(int i = 0; i < BTNSNO; ++i){
        keybase *k = &allkeys[i];
        keyevent e = k->event;
        if(BTN_state(i)){ // key is in pressed state
            switch(e){
                case EVT_NONE: // just pressed
                case EVT_RELEASE:
                    if((k->counter += d) > PRESSTHRESHOLD){
                        k->event = EVT_PRESS;
                    }
                break;
                case EVT_PRESS: // hold
                    if((k->counter += d)> HOLDTHRESHOLD){
                        k->event = EVT_HOLD;
                    }
                break;
                default:
                break;
            }
        }else{ // released
            if(e == EVT_PRESS || e == EVT_HOLD){ // released
                if(k->counter > PRESSTHRESHOLD) k->counter = PRESSTHRESHOLD;
                else if((k->counter -= d) < 0){
                    k->event = EVT_RELEASE; // button released
                }
            }
        }
        if(e != k->event || e == EVT_HOLD){
            k->lastTms = Tms;
            lastUnsleep = Tms;
        }
    }
}

/**
 * @brief keystate - curent key state
 * @param k   - key number
 * @param T   - last event changing time
 * @return key event
 */
keyevent keystate(uint8_t k, uint32_t *T){
    if(k >= BTNSNO) return EVT_NONE;
    keyevent evt = allkeys[k].event;
    // change state `release` to `none` after 1st check
    if(evt == EVT_RELEASE) allkeys[k].event = EVT_NONE;
    if(T) *T = allkeys[k].lastTms;
    return evt;
}

// getter of keyevent for allkeys[]
keyevent keyevt(uint8_t k){
    if(k >= BTNSNO) return EVT_NONE;
    return allkeys[k].event;
}
