/*
 * This file is part of the Servo project.
 * Copyright 2019 Edward Emelianov <eddy@sao.ru>.
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

#include "effects.h"
#include "hardware.h"
#include "usart.h"

static effect_t current_ef[3] = {EFF_NONE, EFF_NONE, EFF_NONE};

#define SPD_STP   (25)

static void eff_madwipe(int n){
    static uint32_t speed[3] = {SPD_STP, SPD_STP, SPD_STP};
    if(onposition(n)){ // move back
        if((speed[n]+=SPD_STP) > SG90_STEP) speed[n] = SPD_STP;
        int val = 0;
        if(getPWM(n) < SG90_MIDPULSE) val = 1;
        setPWM(n, val, speed[n]);
    }
}

static void eff_wipe(int n){
    static uint8_t cntr = 0;
    if(onposition(n)){ // move back
        int val = 0;
        if(getPWM(n) < SG90_MIDPULSE) val = 1;
        if(++cntr < 4){ // stay a little in outermost positions
            setPWM(n, getPWM(n), SG90_STEP/2);
        }else{
            cntr = 0;
            setPWM(n, val, SG90_STEP/2);
        }
    }
}

static void eff_pendulum(int n){
    const uint16_t steps[41] = {0, 10, 21, 33, 47, 62, 79, 97, 117, 140, 165, 193, 224, 258, 295, 337, 383, 434,
                             490, 552, 621, 697, 766, 828, 884, 935, 981, 1023, 1060, 1094, 1125, 1153, 1178,
                             1201, 1221, 1239, 1256, 1271, 1285, 1297, 1308};
    static int8_t cntr = 0, dir = 1;
    if(onposition(n)){
        setPWM(n, SG90_MINPULSE + steps[cntr], SG90_STEP);
        cntr += dir;
        if(cntr == -1){ // min position
            dir = 1;
            cntr = 0; // repeat zero position one time
        }else if(cntr == 41){ // max position
            dir = -1;
            cntr = 40; // and this position needs to repeat too
        }
    }
}

static void eff_pendsm(int n){
    const uint16_t steps[19] = {0, 6, 10, 15, 22, 30, 40, 52, 66, 82, 101, 123, 148, 177, 210, 247, 289, 336, 389};
    static int8_t cntr = 0, dir = 1;
    if(onposition(n)){
        setPWM(n, SG90_MINPULSE + steps[cntr], SG90_STEP);
        cntr += dir;
        if(cntr == -1){ // min position
            dir = 1;
            cntr = 1;
        }else if(cntr == 19){ // max position
            dir = -1;
            cntr = 18;
        }
    }
}

void proc_effect(){
    for(int i = 0; i < 3; ++i){
        switch(current_ef[i]){
            case EFF_WIPE:
                eff_wipe(i);
            break;
            case EFF_MADWIPE:
                eff_madwipe(i);
            break;
            case EFF_PENDULUM:
                eff_pendulum(i);
            break;
            case EFF_SMPENDULUM:
                eff_pendsm(i);
            break;
            case EFF_NONE:
            default:
            break;
        }
    }
}

effect_t set_effect(int n, effect_t eff){
    if(n < 0 || n > 3) return EFF_NONE;
    current_ef[n] = eff;
    return eff;
}
