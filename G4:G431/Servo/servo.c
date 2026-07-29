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

#include <stdint.h>

#include "flash.h"
#include "hardware.h"
#include "servo.h"

static uint16_t servo_tagpos[SERVO_AMOUNT] = {SG90_MIDPULSE, SG90_MIDPULSE, SG90_MIDPULSE, SG90_MIDPULSE};
static uint16_t servo_speed[SERVO_AMOUNT] = {0, 0, 0, 0};

void process_servo(){
    for(uint8_t i = 0; i < SERVO_AMOUNT; ++i){
        uint16_t curpos;
        if(!get_servo(i, &curpos) || !tim_triggered[i]) continue;
        tim_triggered[i] = false;
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
}

// set speed in steps per tick
bool servo_set_speed(uint8_t N, uint16_t s){
    if(N >= SERVO_AMOUNT) return false;
    if(s < 1 || s > the_conf.maxspeed[N]) return false;
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
