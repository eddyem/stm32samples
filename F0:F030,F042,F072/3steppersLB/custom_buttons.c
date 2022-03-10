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


/*
 * check buttons, on long press of button:
 * 1 - switch relay
 * 2 - switch buzzer
 * 3 - work with PWM out 0 (when btn3 pressed, btn1 increased & btn2 decreased PWM width)
 *        press once btn2/3 to change PWM @1, hold to change @25 (repeat as many times as need)
 */
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
