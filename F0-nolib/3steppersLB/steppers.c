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
#include "strfunct.h"

// goto zero stages
typedef enum{
    M0RELAX,      // normal moving
    M0FAST,  // fast move to zero
    M0PLUS,   // move 200 steps +
    M0SLOW   // slowest move to zero
} mvto0state;

typedef enum{
    STALL_NO,       // moving OK
    STALL_ONCE,     // Nstalled < limit
    STALL_STOP      // Nstalled >= limit
} t_stalled;

// motors' direction: 1 for positive, -1 for negative (we need it as could be reverse)
static int8_t motdir[MOTORSNO];
// current position (in steps) by STP counter
static volatile int32_t stppos[MOTORSNO] = {0};
// previous position when check (set to current in start of moving)
static int32_t prevstppos[MOTORSNO];
// target stepper position
static int32_t targstppos[MOTORSNO];
// position to start deceleration
static int32_t decelstartpos[MOTORSNO];
// current encoder position (4 per ticks) (without TIM->CNT)
static volatile int32_t encpos[MOTORSNO] = {0};
// previous encoder position
static int32_t prevencpos[MOTORSNO];
// encoders' ticks per step (calculates @ init)
static int32_t encperstep[MOTORSNO];

// current speed
static uint16_t curspeed[MOTORSNO];
// delta V according to current acceleration & INTERVAL
static uint16_t dV[MOTORSNO];
// ==1 to stop @ nearest step
static uint8_t stopflag[MOTORSNO];
// motor state
static stp_state state[MOTORSNO];
// move to zero state
static mvto0state mvzerostate[MOTORSNO];

// lowest ARR value (highest speed), highest (lowest speed)
//static uint16_t stphighARR[MOTORSNO];
// microsteps=1<<ustepsshift
static uint16_t ustepsshift[MOTORSNO];
// amount of steps for full eceleration/deceleration
static uint32_t accdecsteps[MOTORSNO];

// recalculate ARR according to new speed
TRUE_INLINE void recalcARR(int i){
    mottimers[i]->ARR = (MOTORFREQ / curspeed[i]) >> ustepsshift[i];
}

// run this function after each steppers parameters changing
void init_steppers(){
    timers_setup(); // reinit timers & stop them
    // init variables
    for(int i = 0; i < MOTORSNO; ++i){
        stopflag[i] = 0;
        motdir[i] = 1;
        curspeed[i] = 0;
        dV[i] = the_conf.accel[i] * MOTCHKINTERVAL;
        dV[i] /= 1000; // interval in ms, but accel in steps/s^2
        if(dV[i] == 0) dV[i] = 1;
        state[i] = STP_RELAX;
        ustepsshift[i] = MSB(the_conf.microsteps[i]);
        encperstep[i] = the_conf.encrev[i] / 200;
        if(!the_conf.motflags[i].donthold) MOTOR_EN(i);
        else MOTOR_DIS(i);
    }
}

// get absolute position by encoder
static int32_t encoder_position(uint8_t i){
    int32_t pos = encpos[i];
    if(the_conf.motflags[i].encreverse) pos -= enctimers[i]->CNT;
    else pos += enctimers[i]->CNT;
    return pos;
}

// get current position
errcodes getpos(uint8_t i, int32_t *position){
    if(the_conf.motflags[i].haveencoder){
        *position = encoder_position(i) / encperstep[i];
    }else *position = stppos[i];
    return ERR_OK;
}

errcodes getremainsteps(uint8_t i, int32_t *position){
    *position = targstppos[i] - stppos[i];
    return ERR_OK;
}

// move to absolute position
errcodes motor_absmove(uint8_t i, int32_t newpos){
    //if(i >= MOTORSNO) return ERR_BADPAR; // bad motor number
    if(state[i] != STP_RELAX) return ERR_CANTRUN; // can't move: motor isn't stopping
    if(newpos > (int32_t)the_conf.maxsteps[i] || newpos < -(int32_t)the_conf.maxsteps[i] || newpos == stppos[i])
        return ERR_BADVAL; // too big position or zero
    targstppos[i] = newpos;
    prevencpos[i] = encoder_position(i);
    prevstppos[i] = stppos[i];
    uint8_t inv = the_conf.motflags[i].reverse;
    int32_t delta = newpos - stppos[i];
    if(delta > 0){ // positive direction
        if(delta > 2*(int32_t)accdecsteps[i]){ // can move by trapezoid
            decelstartpos[i] = targstppos[i] - accdecsteps[i];
        }else{ // triangle speed profile
            decelstartpos[i] = stppos[i] + delta/2;
        }
        motdir[i] = 1;
        if(inv) MOTOR_CCW(i);
        else MOTOR_CW(i);
    }else{ // negative direction
        delta = -delta;
        if(delta > 2*(int32_t)accdecsteps[i]){ // can move by trapezoid
            decelstartpos[i] = targstppos[i] + accdecsteps[i];
        }else{ // triangle speed profile
            decelstartpos[i] = stppos[i] - delta/2;
        }
        motdir[i] = -1;
        if(inv) MOTOR_CW(i);
        else MOTOR_CCW(i);
    }
    curspeed[i] = MOTORMINSPEED;
    recalcARR(i);
    MOTOR_EN(i);
    mottimers[i]->CR1 |= TIM_CR1_CEN; // start timer
    state[i] = STP_ACCEL;
    return ERR_OK;
}

// move i'th motor for relsteps
errcodes motor_relmove(uint8_t i, int32_t relsteps){
    return motor_absmove(i, stppos[i] + relsteps);
}

void stopmotor(uint8_t i){
    stopflag[i] = 1;
}

stp_state getmotstate(uint8_t i){
    return state[i];
}

// count steps @tim 14/15/16
void addmicrostep(uint8_t i){
    static volatile uint16_t microsteps[MOTORSNO] = {0}; // current microsteps position
    if(mottimers[i]->SR & TIM_SR_UIF){
        DBG("MOTUP");
        if(ESW_state(i)){ // ESW active
            switch(the_conf.ESW_reaction[i]){
                case ESW_ANYSTOP: // stop motor in any direction
                    stopflag[i] = 1;
                break;
                case ESW_STOPMINUS: // stop only @ minus
                    if(motdir[i] == -1) stopflag[i] = 1;
                break;
                default: // ESW_IGNORE
                break;
            }
        }
        if(++microsteps[i] == the_conf.microsteps[i]){
            microsteps[i] = 0;
            stppos[i] += motdir[i];
            if(stopflag[i] || stppos[i] == targstppos[i]){ // stop NOW
                stopflag[i] = 0;
                mottimers[i]->CR1 &= ~TIM_CR1_CEN; // stop timer
                if(the_conf.motflags[i].donthold)
                    MOTOR_DIS(i); // turn off power
                state[i] = STP_RELAX;
            }
        }
    }
    mottimers[i]->SR = 0;
}

void encoders_UPD(uint8_t i){
    if(enctimers[i]->SR & TIM_SR_UIF){
        int8_t d = 1; // positive (-1 - negative)
        if(enctimers[i]->CR1 & TIM_CR1_DIR) d = -d; // negative
        if(the_conf.motflags[i].encreverse) d = -d;
        if(d == 1) encpos[i] += the_conf.encrev[i];
        else encpos[i] -= the_conf.encrev[i];
    }
    enctimers[i]->SR = 0;
}

// check if motor is stalled
// @return 0 if moving OK,
static t_stalled chkSTALL(uint8_t i){
    if(!the_conf.motflags[i].haveencoder) return STALL_NO;
    static uint8_t Nstalled = 0; // counter of STALL
    int32_t curencpos = encoder_position(i), Denc = curencpos - prevencpos[i];
    int32_t curstppos = stppos[i], Dstp = curstppos - prevstppos[i];
    prevencpos[i] = curencpos;
    if(Dstp == 0){ // veird things
        stopmotor(i);
        return STALL_STOP;
    }
    if(Denc < the_conf.encperstepmin[i]*Dstp || the_conf.encperstepmax[i]*Dstp < Denc){ // stall?
        if(++Nstalled >= NSTALLEDMAX){
            stopflag[i] = 1;
            Nstalled = 0;
            return STALL_STOP;
        }else{
            uint16_t spd = curspeed[i] >> 1; // speed / 2
            curspeed[i] = (spd > MOTORMINSPEED) ? spd : MOTORMINSPEED;
            recalcARR(i);
            if(state[i] == STP_MOVE)
                state[i] = STP_ACCEL;
            return STALL_ONCE;
        }
    }else Nstalled = 0;
    curstppos = curencpos / encperstep[i]; // recalculate current position
    stppos[i] = curstppos;
    prevstppos[i] = curstppos;
    return STALL_NO;
}

// check state of i`th stepper
static void chkstepper(int i){
    int32_t newspeed;
    switch(state[i]){
        case STP_ACCEL: // acceleration to max speed
            if(STALL_NO == chkSTALL(i)){
                newspeed = curspeed[i] + dV[i];
                if(newspeed >= the_conf.maxspd[i]){ // max speed reached -> move with it
                    curspeed[i] = the_conf.maxspd[i];
                    state[i] = STP_MOVE;
                }else{ // increase speed
                    curspeed[i] = newspeed;
                }
                recalcARR(i);
            }
            // check position for triangle profile
            if(motdir[i] > 0){
                if(stppos[i] >= decelstartpos[i]) // reached end of acceleration
                    state[i] = STP_DECEL;
            }else{
                if(stppos[i] <= decelstartpos[i])
                    state[i] = STP_DECEL;
            }
        break;
        case STP_MOVE: // move @ constant speed until need to decelerate
            if(STALL_NO == chkSTALL(i)){
                // check position
                if(motdir[i] > 0){
                    if(stppos[i] >= decelstartpos[i]) // reached start of deceleration
                        state[i] = STP_DECEL;
                }else{
                    if(stppos[i] <= decelstartpos[i])
                        state[i] = STP_DECEL;
                }
            }
        break;
        case STP_DECEL:
            if(STALL_NO == chkSTALL(i)){
                newspeed = curspeed[i] - dV[i];
                if(newspeed > MOTORMINSPEED){
                    curspeed[i] = newspeed;
                }else{
                    curspeed[i] = MOTORMINSPEED;
                    state[i] = STP_MVSLOW;
                }
                recalcARR(i);
            }
            // fallthrough
        case STP_MVSLOW:
            if(motdir[i] > 0){
                if(stppos[i] >= targstppos[i]) // reached start of deceleration
                    stopflag[i] = 0;
            }else{
                if(stppos[i] <= targstppos[i])
                    stopflag[i] = 0;
            }
        break;
        default: // RELAX, STALL, ERR -> do nothing
            return;
    }
    switch(mvzerostate[i]){
        case M0FAST:
            if(state[i] == STP_RELAX){ // stopped -> move to +
                if(ERR_OK != motor_relmove(i, 50)){
                    state[i] = STP_ERR;
                    mvzerostate[i] = M0RELAX;
                }else
                    mvzerostate[i] = M0PLUS;
            }
        break;
        case M0PLUS:
            if(state[i] == STP_RELAX){ // stopped -> move
                if(ERR_OK != motor_relmove(i, -100)){
                    state[i] = STP_ERR;
                    mvzerostate[i] = M0RELAX;
                }else{
                    state[i] = STP_MVSLOW;
                    mvzerostate[i] = M0SLOW;
                }
            }
        break;
        case M0SLOW:
            if(state[i] == STP_RELAX){
                encpos[i] = 0;
                stppos[i] = 0;
                mottimers[i]->CNT = 0; // set encoder counter to zero
                mvzerostate[i] = M0RELAX;
            }
        break;
        default: // RELAX: do nothing
        break;
    }
}

void process_steppers(){
    static uint32_t Tlast = 0;
    if(Tms - Tlast < MOTCHKINTERVAL) return; // hit every 10ms
    Tlast = Tms;
    for(int i = 0; i < MOTORSNO; ++i){
        chkstepper(i);
    }
}
