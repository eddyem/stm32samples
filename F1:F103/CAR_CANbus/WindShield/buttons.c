/*
 * This file is part of the windshield project.
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

#include <stm32f1.h>

#include "buttons.h"
#include "hardware.h"
#ifdef EBUG
#include "usart.h"
#endif

extern volatile uint32_t Tms;
//uint32_t lastUnsleep = 0;

// threshold in ms for press/hold
#define PRESSTHRESHOLD  (29)
// HOLDTHRESHOLD = PRESSTHRESHOLD + hold time
#define HOLDTHRESHOLD   (329)
/*
typedef struct{
    keyevent event;         // current key event
    int16_t counter;        // press/release milliseconds counter
    GPIO_TypeDef *port;     // key port
    uint16_t pin;           // key pin
    uint8_t changed : 1;    // the event have been changed
    uint8_t inverted : 1;   // button inverted: 0 - passive, 1 - active
} keybase;
*/
// HALLs are inverted as disconnected when active
keybase allkeys[KEY_AMOUNT] = {
    [HALL_D] = {.port = HALL_PORT, .pin = HALL_D_PIN, .inverted = 1},
    [HALL_U] = {.port = HALL_PORT, .pin = HALL_U_PIN, .inverted = 1},
    [KEY_D] = {.port = BUTTON_PORT, .pin = BUTTON_D_PIN},
    [KEY_U] = {.port = BUTTON_PORT, .pin = BUTTON_U_PIN},
    [DIR_D] = {.port = DIR_PORT, .pin = DIR_D_PIN, .inverted = 1},
    [DIR_U] = {.port = DIR_PORT, .pin = DIR_U_PIN, .inverted = 1}
};

// return 1 if something was changed
int process_keys(){
    static uint32_t lastT = 0;
    int changed = FALSE;
    if(Tms == lastT) return FALSE;
    uint16_t d = (uint16_t)(Tms - lastT);
    lastT = Tms;
    for(int i = 0; i < KEY_AMOUNT; ++i){
        keybase *k = &allkeys[i];
        keyevent e = k->event;
        if(PRESSED(k->port, k->pin) != k->inverted){ // key is in pressed state
            //lastUnsleep = Tms; // update activity time (any key is in pressed state)
            switch(e){
                case EVT_NONE: // just pressed
                case EVT_RELEASE:
                    k->counter = PRESSTHRESHOLD; // anti-bounce for released state
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
        if(e != k->event){
            k->changed = 1;
            changed = TRUE;
        }
    }
    return changed;
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
