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

#include "flash.h"
#include "hardware.h"
#include "steppers.h"

int8_t motdir[MOTORSNO]; // motors' direction: 1 for positive, -1 for negative (we need it as could be reverse)

static volatile int32_t stppos[MOTORSNO]; // current position (in steps) by STP counter
static int32_t encpos[MOTORSNO]; // current encoder position (4 per ticks) minus TIM->CNT
static int32_t stepsleft[MOTORSNO]; // steps left to stop (full steps or encoder's counters)

static uint16_t microsteps[MOTORSNO] = {0}; // current microsteps position
static uint8_t stopflag[MOTORSNO]; // ==1 to stop @ nearest step

void init_steppers(){
    // init variables
    for(int i = 0; i < MOTORSNO; ++i){
        mottimers[i]->CR1 &= ~TIM_CR1_CEN;
        stepsleft[i] = 0;
        microsteps[i] = 0;
        stopflag[i] = 0;
        motdir[i] = 1;
        stppos[i] = 0;
    }
    timers_setup();
}

// get position
errcodes getpos(int i, int32_t *position){
    if(the_conf.motflags[i].haveencoder){
        *position = encpos[i] + enctimers[i]->CNT;
    }else *position = stppos[i];
    return ERR_OK;
}
// set position
errcodes setpos(int i, int32_t newpos){
    if(newpos < -(int32_t)the_conf.maxsteps[i] || newpos > (int32_t)the_conf.maxsteps[i])
        return ERR_BADPAR;
    int32_t diff = newpos - stppos[i];
    if(diff == 0) return ERR_OK;
    if(diff > 0) motdir[i] = 1;
    else{
        diff = -diff;
        motdir[i] = -1;
    }
    // TODO: set direction pin, run timer, set target position
    // depends on the_conf.motflags[i].reverse
    return ERR_OK;
}

void stopmotor(int i){
    stopflag[i] = 1;
    stepsleft[i] = 0;
    microsteps[i] = 0;
}

// count steps @tim 14/15/16
void addmicrostep(int i){
    if(mottimers[i]->SR & TIM_SR_UIF){
        if(++microsteps[i] == the_conf.microsteps[i]){
            microsteps[i] = 0;
            if(stopflag[i]){ // stop NOW
                stopflag[i] = 0;
                mottimers[i]->CR1 &= ~TIM_CR1_CEN; // stop timer
            }
            stppos[i] += motdir[i];
        }
    }
    mottimers[i]->SR = 0;
}

void encoders_UPD(int i){
    if(enctimers[i]->SR & TIM_SR_UIF){
        int8_t d = 1; // positive (-1 - negative)
        if(enctimers[i]->CR1 & TIM_CR1_DIR) d = -d; // negative
        if(the_conf.motflags[i].encreverse) d = -d;
        if(d == 1) encpos[i] += the_conf.encrev[i];
        else encpos[i] -= the_conf.encrev[i];
    }
    enctimers[i]->SR = 0;
}
