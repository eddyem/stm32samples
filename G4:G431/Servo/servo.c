/*
 * This file is part of the servo project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32g4.h>
#include <stdint.h>

#include "flash.h"
#include "hardware.h"
#include "servo.h"

static uint16_t servo_tagpos[SERVO_AMOUNT] = {SG90_MIDPULSE, SG90_MIDPULSE, SG90_MIDPULSE, SG90_MIDPULSE};
static uint16_t servo_speed[SERVO_AMOUNT] = {0, 0, 0, 0};

void process_servo(){
    static bool notinited = true;
#if SERVO_AMOUNT != 4
#error "Add timers' variables"
#endif
    static const uint32_t trgflags[SERVO_AMOUNT] = {TIM_SR_CC1IF, TIM_SR_CC2IF, TIM_SR_CC3IF, TIM_SR_CC4IF};
    if(notinited){ // init tagpos with starting values from settings at first run
        for(int i = 0; i < SERVO_AMOUNT; ++i) servo_tagpos[i] = the_conf.startpulse[i];
        notinited = false;
    }
    uint32_t sr = TIM3->SR;
    uint32_t clear_mask = 0;
    for(uint8_t i = 0; i < SERVO_AMOUNT; ++i){
        uint16_t curpos;
        if(!get_servo(i, &curpos) || !(sr & trgflags[i])) continue;
        clear_mask |= trgflags[i];
        if(servo_tagpos[i] == curpos || servo_speed[i] < 1) continue;
        int32_t newpos = curpos;
        if(servo_tagpos[i] > curpos){
            newpos += servo_speed[i];
            if(newpos > servo_tagpos[i]) newpos = servo_tagpos[i];
        }else{
            newpos -= servo_speed[i];
            if(newpos < servo_tagpos[i]) newpos = servo_tagpos[i];
        }
        set_servo(i, (uint16_t) newpos);
    }
    if(clear_mask) TIM3->SR &= ~clear_mask;
}

// set speed in steps per tick
bool servo_set_speed(uint8_t N, uint16_t s){
    if(N >= SERVO_AMOUNT) return false;
    if(s > the_conf.maxspeed[N]) return false; // let s==0: this will allow to stop mowing to tagpos
    servo_speed[N] = s;
    return true;
}

bool servo_get_speed(uint8_t N, uint16_t *s){
    if(N >= SERVO_AMOUNT) return false;
    if(s) *s = servo_speed[N];
    return true;
}

bool servo_set_tagpos(uint8_t N, uint16_t pos){
    if(N >= SERVO_AMOUNT) return false;
    if(pos < the_conf.minpulse[N] || pos > the_conf.maxpulse[N]) return false;
    servo_tagpos[N] = pos;
    return true;
}

bool servo_get_tagpos(uint8_t N, uint16_t *pos){
    if(N >= SERVO_AMOUNT) return false;
    if(pos) *pos = servo_tagpos[N];
    return true;
}
