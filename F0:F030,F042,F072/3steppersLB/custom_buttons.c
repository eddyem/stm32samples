/*
 * This file is part of the 3steppers project.
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

// custom standalone buttons reaction

#include <stm32f0.h>
#include "buttons.h"
#include "custom_buttons.h"
#include "hardware.h"
#include "steppers.h"
#include "strfunct.h"

/**
 * @brief custom_buttons_process - check four buttons and if find hold:
 * 0..2 - move motor i by +maxsteps while hold
 * 3 (or pressed) - switch moving direction to -
 * if pressed:
 * 0..2 - stop motor i (if moving) or move by +10 steps
 */
void custom_buttons_process(){
    static uint32_t lastT = 0;
    static keyevent lastevent[3] = {EVT_NONE, EVT_NONE, EVT_NONE};
    if(lastUnsleep == lastT) return; // no buttons activity
    lastT = lastUnsleep;
    int32_t dir = 1;
    if(keyevt(3) == EVT_HOLD || keyevt(3) == EVT_PRESS) dir = -1; // button 3: change direction to `-`
    for(int i = 0; i < 3; ++i){
        keyevent e = keyevt(i);
        if(e == EVT_RELEASE){ // move by 10 steps or emergency stop @ release after shot press
            if(lastevent[i] == EVT_PRESS){
                if(getmotstate(i) == STP_RELAX) motor_relslow(i, dir*10);
                else emstopmotor(i);
            }else stopmotor(i); // stop motor when key was released after long hold
        }else if(e == EVT_HOLD){ // move by `maxsteps` steps
            if(getmotstate(i) == STP_RELAX){
                if(ERR_OK != motor_absmove(i, dir*the_conf.maxsteps[i])){
                    // here we can do BEEP
#ifdef EBUG
                    SEND("can't move\n");
#endif
                }
            }
        }
        lastevent[i] = e;
    }
}


/*
 * check buttons, on long press of button:
 * 1 - switch relay
 * 2 - switch buzzer
 * 3 - work with PWM out 0 (when btn3 pressed, btn1 increased & btn2 decreased PWM width)
 *        press once btn2/3 to change PWM @1, hold to change @25 (repeat as many times as need)
 *
void custom_buttons_process(){
    static uint32_t lastT = 0;
    static uint8_t pwmval = 127;
    static uint8_t trig = 0; // == 1 if given btn3 was off
    if(lastUnsleep == lastT) return; // no buttons activity
    lastT = lastUnsleep;
    if(keyevt(3) == EVT_HOLD){ // PWM
        if(keyevt(2) == EVT_HOLD){ // decrease PWM by 25
            if(pwmval > 25) pwmval -= 25;
            else pwmval = 0;
        }else if(keyevt(2) == EVT_PRESS){ // decrease PWM by 1
            if(pwmval > 0) --pwmval;
        }else if(keyevt(1) == EVT_HOLD){ // increase PWM by 25
            if(pwmval < 230) pwmval += 25;
            else pwmval = 255;
        }else if(keyevt(1) == EVT_PRESS){
            if(pwmval < 254) ++pwmval;
        }
        if(trig == 0){ // first hold after release
            if(TIM1->CCR1) TIM1->CCR1 = 0; // turn off if was ON
            else{
                TIM1->CCR1 = pwmval;
                trig = 1;
            }
        }else TIM1->CCR1 = pwmval;
        return;
    }else trig = 0;
    if(keyevt(1) == EVT_HOLD){ // relay
        TGL(RELAY);
    }
    if(keyevt(2) == EVT_HOLD){ // buzzer
        TGL(BUZZER);
    }
}
*/
