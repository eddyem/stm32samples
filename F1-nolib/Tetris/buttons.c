/*
 * This file is part of the TETRIS project.
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

#include <stm32f1.h>

#include "buttons.h"
#include "hardware.h"

extern volatile uint32_t Tms;

// threshold in ms for press/hold
#define PRESSTHRESHOLD  (9)
#define HOLDTHRESHOLD   (249)

typedef struct{
    keycode code;           // key code
    keyevent event;         // current key event
    int16_t counter;        // press/release counter
    GPIO_TypeDef *port;     // key port
    uint16_t pin;           // key pin
    uint8_t changed;        // the event have been changed
} keybase;

keybase allkeys[KEY_AMOUNT] = {
    [KEY_L] = {.port = JLEFT_port,  .pin = JLEFT_pin},
    [KEY_R] = {.port = JRIGHT_port, .pin = JRIGHT_pin},
    [KEY_U] = {.port = JUP_port,    .pin = JUP_pin},
    [KEY_D] = {.port = JDOWN_port,  .pin = JDOWN_pin},
    [KEY_M] = {.port = JMENU_port,  .pin = JMENU_pin},
    [KEY_S] = {.port = JENTER_port, .pin = JENTER_pin}
};

void process_keys(){
    static uint32_t lastT = 0;
    if(Tms == lastT) return;
    uint16_t d = (uint16_t)(Tms - lastT);
    lastT = Tms;
    for(int i = 0; i < KEY_AMOUNT; ++i){
        keybase *k = &allkeys[i];
        keyevent e = k->event;
        if(PRESSED(k->port, k->pin)){ // key is in pressed state
            switch(e){
                case EVT_NONE: // just pressed
                case EVT_RELEASE:
                    if((k->counter += d) > PRESSTHRESHOLD)
                        k->event = EVT_PRESS;
                break;
                case EVT_PRESS: // hold
                    if((k->counter += d)> HOLDTHRESHOLD)
                        k->event = EVT_HOLD;
                break;
                default:
                break;
            }
        }else{ // released
            switch(e){
                case EVT_PRESS: // count -> none
                    if(k->counter > PRESSTHRESHOLD) k->counter = PRESSTHRESHOLD;
                    else if((k->counter -= d) < 0) k->event = EVT_NONE; // button released
                break;
                case EVT_HOLD:  // count -> release
                    if(k->counter > PRESSTHRESHOLD) k->counter = PRESSTHRESHOLD;
                    else if((k->counter -= d) < 0) k->event = EVT_RELEASE; // button released
                break;
                default:
                break;
            }
        }
        if(e != k->event) k->changed = 1;
    }
}

/**
 * @brief keystate curent key state
 * @param k   - key code
 * @param evt - its event
 * @return 1 if event was changed since last call
 */
uint8_t keystate(keycode k, keyevent *evt){
    if(k >= KEY_AMOUNT) return EVT_NONE;
    *evt = allkeys[k].event;
    // change state `release` to `none` after 1st check (or will be `changed` @ release -> none)
    if(*evt == EVT_RELEASE) allkeys[k].event = EVT_NONE;
    uint8_t r = allkeys[k].changed;
    allkeys[k].changed = 0;
    return  r;
}

keyevent keyevt(keycode k){
    if(k >= KEY_AMOUNT) return EVT_NONE;
    return allkeys[k].event;
}

// clear all `changed` states
void clear_events(){
    for(int i = 0; i < KEY_AMOUNT; ++i)
        allkeys[i].changed = 0;
}
