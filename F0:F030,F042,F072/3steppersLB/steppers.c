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
    M0RELAX,        // normal moving
    M0FAST,         // fast move to zero
    M0SLOW          // slowest move from ESW
} mvto0state;

typedef enum{
    STALL_NO,       // moving OK
    STALL_ONCE,     // Nstalled < limit
    STALL_STOP      // Nstalled >= limit
} t_stalled;

static t_stalled stallflags[MOTORSNO];

// motors' direction: 1 for positive, -1 for negative (we need it as could be reverse)
static int8_t motdir[MOTORSNO];
// direction of moving when stalled (forbid moving in that direction before go out of position)
static int8_t stalleddir[MOTORSNO] = {0};
// current position (in steps) by STP counter
static volatile int32_t stppos[MOTORSNO] = {0};
// previous position when check (set to current in start of moving)
static int32_t prevstppos[MOTORSNO];
// target stepper position
static int32_t targstppos[MOTORSNO] = {0};
// position to start deceleration
static int32_t decelstartpos[MOTORSNO];
// current encoder position (4 per ticks) (without TIM->CNT)
static volatile int32_t encpos[MOTORSNO] = {0};
// previous encoder position
static int32_t prevencpos[MOTORSNO] = {0};
// encoders' ticks per step (calculates @ init)
static int32_t encperstep[MOTORSNO];
// ESW reaction - local copy
static uint8_t ESW_reaction[MOTORSNO];

// current speed
static uint16_t curspeed[MOTORSNO];
static uint16_t startspeed[MOTORSNO]; // speed when deceleration starts
// ==1 to stop @ nearest step
static uint8_t stopflag[MOTORSNO];
// motor state
static stp_state state[MOTORSNO];
// move to zero state
static mvto0state mvzerostate[MOTORSNO];

static int8_t Nstalled[MOTORSNO] = {0}; // counter of STALL

// lowest ARR value (highest speed), highest (lowest speed)
//static uint16_t stphighARR[MOTORSNO];
// microsteps=1<<ustepsshift
static uint16_t ustepsshift[MOTORSNO];
// amount of steps for full acceleration/deceleration
static uint32_t accdecsteps[MOTORSNO];

// time when acceleration or deceleration starts
static uint32_t Taccel[MOTORSNO] = {0};

// recalculate ARR according to new speed
TRUE_INLINE void recalcARR(int i){
    uint32_t ARR = (((PCLK/(MOTORTIM_PSC+1)) / curspeed[i]) >> ustepsshift[i]) - 1;
    if(ARR < MOTORTIM_ARRMIN) ARR = MOTORTIM_ARRMIN;
    else if(ARR > 0xffff) ARR = 0xffff;
    mottimers[i]->ARR = ARR;
    curspeed[i] = (((PCLK/(MOTORTIM_PSC+1)) / (ARR+1)) >> ustepsshift[i]); // recalculate speed due to new val
}

// run this function after each steppers parameters changing
void init_steppers(){
    timers_setup(); // reinit timers & stop them
    // init variables
    for(int i = 0; i < MOTORSNO; ++i){
        stalleddir[i] = 0; // clear old stall direction
        stopflag[i] = 0;
        motdir[i] = 0;
        curspeed[i] = 0;
        accdecsteps[i] = (the_conf.maxspd[i] * the_conf.maxspd[i]) / the_conf.accel[i] / 2;
        state[i] = STP_RELAX;
        ustepsshift[i] = MSB(the_conf.microsteps[i]);
        encperstep[i] = the_conf.encrev[i] / STEPSPERREV;
        if(!the_conf.motflags[i].donthold) MOTOR_EN(i);
        else MOTOR_DIS(i);
        ESW_reaction[i] = the_conf.ESW_reaction[i];
    }
}

// get absolute position by encoder
int32_t encoder_position(uint8_t i){
    int32_t pos = encpos[i];
    if(the_conf.motflags[i].encreverse) pos -= enctimers[i]->CNT;
    else pos += enctimers[i]->CNT;
    return pos;
}

// set encoder's position, return 0 if false (motor's state != relax)
int setencpos(uint8_t i, int32_t position){
    if(state[i] != STP_RELAX) return FALSE;
    int32_t remain = position % the_conf.encrev[i];
    if(remain < 0) remain += the_conf.encrev[i];
    enctimers[i]->CNT = remain;
    prevencpos[i] = encpos[i] = position - remain;
    int32_t curstppos;
    getpos(i, &curstppos);
    stppos[i] = targstppos[i] = curstppos;
#ifdef EBUG
    SEND("MOTOR"); bufputchar('0'+i); SEND(", position="); printi(position); SEND(", remain=");
    printi(remain); SEND(", CNT="); printu(enctimers[i]->CNT); SEND(", pos="); printi(encpos[i]); NL();
#endif
    return TRUE;
}

// set absolute position of motor `i`
errcodes setmotpos(uint8_t i, int32_t position){
    if(state[i] != STP_RELAX) return ERR_CANTRUN;
    if(position > (int32_t)the_conf.maxsteps[i] || position < -(int32_t)the_conf.maxsteps[i])
        return ERR_BADVAL; // too big position or zero
    if(position == stppos[i]) return ERR_OK;
    setencpos(i, position * encperstep[i]);
    return ERR_OK;
}

// get current position
errcodes getpos(uint8_t i, int32_t *position){
    if(the_conf.motflags[i].haveencoder){
        int32_t p = encoder_position(i);
        if(p < 0) p -= encperstep[i]>>1;
        else p += encperstep[i]>>1;
        p /= encperstep[i];
        *position = p;
    }else
        *position = stppos[i];
    return ERR_OK;
}

errcodes getremainsteps(uint8_t i, int32_t *position){
    *position = targstppos[i] - stppos[i];
    return ERR_OK;
}

// calculate acceleration/deceleration parameters for motor i
static void calcacceleration(uint8_t i){
    int32_t delta = targstppos[i] - stppos[i];
    if(delta > 0){ // positive direction
        if(delta > 2*(int32_t)accdecsteps[i]){ // can move by trapezoid
            decelstartpos[i] = targstppos[i] - accdecsteps[i];
        }else{ // triangle speed profile
            decelstartpos[i] = stppos[i] + delta/2;
        }
        if(the_conf.motflags[i].reverse) MOTOR_CCW(i);
        else MOTOR_CW(i);
    }else{ // negative direction
        delta = -delta;
        if(delta > 2*(int32_t)accdecsteps[i]){ // can move by trapezoid
            decelstartpos[i] = targstppos[i] + accdecsteps[i];
        }else{ // triangle speed profile
            decelstartpos[i] = stppos[i] - delta/2;
        }
        if(the_conf.motflags[i].reverse) MOTOR_CW(i);
        else MOTOR_CCW(i);
    }
    if(state[i] != STP_MVSLOW) state[i] = STP_ACCEL;
    startspeed[i] = curspeed[i];
    Taccel[i] = Tms;
    recalcARR(i);
}

// check if end-switch is blocking the moving of i'th motor
// @return TRUE if motor can't move
static int esw_block(uint8_t i){
    int ret = FALSE;
    if(ESW_state(i)){ // ESW active
        switch(ESW_reaction[i]){
            case ESW_ANYSTOP: // stop motor in any direction
                ret = TRUE;
            break;
            case ESW_STOPMINUS: // stop only @ minus
                if(motdir[i] == -1) ret = TRUE;
            break;
            default: // ESW_IGNORE
            break;
        }
    }
    return ret;
}

// move to absolute position
errcodes motor_absmove(uint8_t i, int32_t newpos){
    //if(i >= MOTORSNO) return ERR_BADPAR; // bad motor number
    int8_t dir = (newpos > stppos[i]) ? 1 : -1;
    switch(state[i]){
        case STP_ERR:
        case STP_RELAX:
        break;
        case STP_STALL:
            if(dir == stalleddir[i]){
                DBG("Move to stalled direction!");
                return ERR_CANTRUN; // can't run into stalled direction
            }
        break;
        default: // moving state
            DBG("Always moving");
            return ERR_CANTRUN;
    }
    if(newpos > (int32_t)the_conf.maxsteps[i] || newpos < -(int32_t)the_conf.maxsteps[i] || newpos == stppos[i]){
        DBG("Too much steps");
        return ERR_BADVAL; // too big position or zero
    }
    motdir[i] = dir;
    if(esw_block(i)){
        DBG("Block by ESW");
        return ERR_CANTRUN; // on end-switch
    }
    Nstalled[i] = (state[i] == STP_STALL) ? -(NSTALLEDMAX*4) : 0; // give some more chances to go out of stall state
    stopflag[i] = 0;
    targstppos[i] = newpos;
    prevencpos[i] = encoder_position(i);
    prevstppos[i] = stppos[i];
    curspeed[i] = the_conf.minspd[i];
    calcacceleration(i);
#ifdef EBUG
    SEND("MOTOR"); bufputchar('0'+i);
    SEND(" targstppos="); printi(targstppos[i]);
    SEND(", decelstart="); printi(decelstartpos[i]);
    SEND(", accdecsteps="); printu(accdecsteps[i]); NL();
#endif
    MOTOR_EN(i);
    mottimers[i]->CR1 |= TIM_CR1_CEN; // start timer
    return ERR_OK;
}

// move i'th motor for relsteps
errcodes motor_relmove(uint8_t i, int32_t relsteps){
    return motor_absmove(i, stppos[i] + relsteps);
}

errcodes motor_relslow(uint8_t i, int32_t relsteps){
    errcodes e = motor_absmove(i, stppos[i] + relsteps);
    if(ERR_OK == e){
        state[i] = STP_MVSLOW;
    }
    return e;
}

// emergency stop and clear errors
void emstopmotor(uint8_t i){
    switch(state[i]){
        case STP_ERR:   // clear error state
        case STP_STALL:
            state[i] = STP_RELAX;
        // fallthrough
        case STP_RELAX: // do nothing in stopping state
            return;
        default:
        break;
    }
    stopflag[i] = 1;
}

stp_state getmotstate(uint8_t i){
    return state[i];
}

// count steps @tim 14/15/16
void addmicrostep(uint8_t i){
    static volatile uint16_t microsteps[MOTORSNO] = {0}; // current microsteps position
    if(esw_block(i)) stopflag[i] = 1; // turn on stop flag if end-switch was active
    if(++microsteps[i] == the_conf.microsteps[i]){
        microsteps[i] = 0;
        stppos[i] += motdir[i];
        uint8_t stop_at_pos = 0;
        if(motdir[i] > 0){
            if(stppos[i] >= targstppos[i]){ // reached stop position
                stop_at_pos = 1;
            }
        }else{
            if(stppos[i] <= targstppos[i]){
                stop_at_pos = 1;
            }
        }
        if(stopflag[i] || stop_at_pos){ // stop NOW
            if(stopflag[i]) targstppos[i] = stppos[i]; // keep position (for keep flag)
            stopflag[i] = 0;
            mottimers[i]->CR1 &= ~TIM_CR1_CEN; // stop timer
            if(the_conf.motflags[i].donthold)
                MOTOR_DIS(i); // turn off power
            if(stallflags[i] == STALL_STOP){
                stallflags[i] = STALL_NO;
                state[i] = STP_STALL;
            }else
                state[i] = STP_RELAX;
#ifdef EBUG
            SEND("MOTOR"); bufputchar('0'+i); SEND(" stop @"); printi(stppos[i]); newline();
#endif
        }
    }
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
    int32_t curencpos = encoder_position(i), Denc = curencpos - prevencpos[i];
    int32_t curstppos = stppos[i], Dstp = curstppos - prevstppos[i];
    int difsign = 1;
    if(Dstp < 0){
        Dstp = -Dstp;
        difsign = -difsign;
    }
    if(Dstp < STALLEDSTEPS){ // didn't move even @ `STALLEDSTEPS` steps
        stallflags[i] = STALL_NO;
        return STALL_NO;
    }
    if(Denc < 0){
        Denc = -Denc;
        difsign = -difsign;
    }
    if(difsign == -1){ // motor and encÏder moves to different sides!!!
        Denc = -Denc; // init STALL state
    }
    prevencpos[i] = curencpos;
    // TODO: check if it should be here
    getpos(i, &curstppos); // recalculate current position
    stppos[i] = curstppos;
    prevstppos[i] = curstppos;
    if(Denc < the_conf.encperstepmin[i]*Dstp || the_conf.encperstepmax[i]*Dstp < Denc){ // stall?
#ifdef EBUG
        SEND("MOTOR"); bufputchar('0'+i); SEND(" Denc="); printi(Denc); SEND(", Dstp="); printu(Dstp);
        SEND(", speed="); printu(curspeed[i]);
#endif
        if(++Nstalled[i] > NSTALLEDMAX){
            stopflag[i] = 1;
            Nstalled[i] = 0;
#ifdef EBUG
            SEND("  ---  STALL!");
#else
            SEND("ERRCODE="); bufputchar(ERR_CANTRUN+'0');
            SEND("\nstate"); bufputchar(i+'0'); bufputchar('=');
            bufputchar(STP_STALL);
#endif
            NL();
            stallflags[i] = STALL_STOP;
            stalleddir[i] = motdir[i];
            return STALL_STOP;
        }else{
            uint16_t spd = curspeed[i] >> 1; // speed / 2
            curspeed[i] = (spd > the_conf.minspd[i]) ? spd : the_conf.minspd[i];
            // now recalculate acc/dec parameters
            calcacceleration(i);
#ifdef EBUG
            SEND("  ---  pre-stall, newspeed="); printu(curspeed[i]); NL();
#endif
            stallflags[i] = STALL_ONCE;
            return STALL_ONCE;
        }
    }
    Nstalled[i] = 0;
    return STALL_NO;
}

#ifdef EBUG
#define TODECEL() do{state[i] = STP_DECEL;  \
        startspeed[i] = curspeed[i];        \
        Taccel[i] = Tms;                    \
        SEND("MOTOR"); bufputchar('0'+i);   \
        SEND("  -> DECEL@"); printi(stppos[i]); SEND(", V="); printu(curspeed[i]); NL(); \
        }while(0)
#else
#define TODECEL() do{state[i] = STP_DECEL;  \
        startspeed[i] = curspeed[i];        \
        Taccel[i] = Tms;                    \
        }while(0)
#endif

// check state of i`th stepper
static void chkstepper(int i){
    int32_t i32;
    static uint8_t stopctr[3] = {0}; // counters for encoders/position zeroing after stopping @ esw
    switch(state[i]){
        case STP_RELAX: // check if need to keep current position
            if(the_conf.motflags[i].haveencoder){
                getpos(i, &i32);
                int32_t diff = stppos[i] - i32; // correct `curpos` counter by encoder
                if(diff){ // correct current stppos by encoder
#ifdef EBUG
                    SEND("MOTOR"); bufputchar('0'+i);
                    SEND(" diff="); printi(diff);
                    SEND(", change stppos from "); printi(stppos[i]); SEND(" to "); printi(i32); NL();
#endif
                    stppos[i] = i32;
                }
                if(the_conf.motflags[i].keeppos){ // keep old position
                    diff = targstppos[i] - i32; // check whether we need to change position
                    if(diff){ // try to correct position
#ifdef EBUG
                        SEND("MOTOR"); bufputchar('0'+i);
                        SEND(" curpos="); printi(i32); SEND(", need="); printi(targstppos[i]); NL();
#endif
                        if(ERR_OK != motor_absmove(i, targstppos[i]))
                            targstppos[i] = i32; // can't move to target position - forget it
                    }
                }
            }
        break;
        case STP_ACCEL: // acceleration to max speed
            if(STALL_NO == chkSTALL(i)){
                //newspeed = curspeed[i] + dV[i];
                i32 = the_conf.minspd[i] + (the_conf.accel[i] * (Tms - Taccel[i])) / 1000;
                if(i32 >= the_conf.maxspd[i]){ // max speed reached -> move with it
                    curspeed[i] = the_conf.maxspd[i];
                    state[i] = STP_MOVE;
#ifdef EBUG
                    SEND("MOTOR"); bufputchar('0'+i);
                    SEND("  -> MOVE@"); printi(stppos[i]); SEND(", V="); printu(curspeed[i]); NL();
#endif
                }else{ // increase speed
                    curspeed[i] = i32;
                }
                recalcARR(i);
            }
            // check position for triangle profile
            if(motdir[i] > 0){
                if(stppos[i] >= decelstartpos[i]){ // reached end of acceleration
                    TODECEL();
                }
            }else{
                if(stppos[i] <= decelstartpos[i]){
                    TODECEL();
                }
            }
        break;
        case STP_MOVE: // move @ constant speed until need to decelerate
            if(STALL_NO == chkSTALL(i)){
                // check position
                if(motdir[i] > 0){
                    if(stppos[i] >= decelstartpos[i]){ // reached start of deceleration
                        TODECEL();
                    }
                }else{
                    if(stppos[i] <= decelstartpos[i]){
                        TODECEL();
                    }
                }
            }
        break;
        case STP_DECEL:
            if(STALL_NO == chkSTALL(i)){
                //newspeed = curspeed[i] - dV[i];
                i32 = startspeed[i] - (the_conf.accel[i] * (Tms - Taccel[i])) / 1000;
                if(i32 > the_conf.minspd[i]){
                    curspeed[i] = i32;
                }else{
                    curspeed[i] = the_conf.minspd[i];
                    state[i] = STP_MVSLOW;
#ifdef EBUG
                    SEND("MOTOR"); bufputchar('0'+i);
                    SEND("  -> MVSLOW@"); printi(stppos[i]); NL();
#endif
                }
                recalcARR(i);
                //SEND("spd="); printu(curspeed[i]); SEND(", pos="); printi(stppos[i]); newline();
            }
        break;
        case STP_MVSLOW:
            chkSTALL(i);
        break;
        default: // STALL, ERR -> do nothing, check mvzerostate
        break;
    }
    switch(mvzerostate[i]){
        case M0FAST:
            if(state[i] == STP_RELAX || state[i] == STP_STALL){ // stopped -> move to +
#ifdef EBUG
                SEND("M0FAST: motor stopped\n");
#endif
                if(ERR_OK != motor_relslow(i, 1000)){
#ifdef EBUG
                    SEND("Can't move\n");
#endif
                    state[i] = STP_ERR;
                    mvzerostate[i] = M0RELAX;
                    ESW_reaction[i] = the_conf.ESW_reaction[i];
                }else{
                    mvzerostate[i] = M0SLOW;
                    stopctr[i] = 0;
                }
            }
        break;
        case M0SLOW:
            if(0 == ESW_state(i)){ // moved out of limit switch - can stop
                emstopmotor(i);
            }
            if((state[i] == STP_RELAX || state[i] == STP_STALL) && ++stopctr[i] > 5){ // wait at least 50ms
#ifdef EBUG
                SEND("M0SLOW: set position to 0\n"); NL();
#endif
                ESW_reaction[i] = the_conf.ESW_reaction[i];
                prevencpos[i] = encpos[i] = 0;
                targstppos[i] = stppos[i] = 0;
                enctimers[i]->CNT = 0; // set encoder counter to zero
                mvzerostate[i] = M0RELAX;
            }
        break;
        default: // RELAX, STALL: do nothing
        break;
    }
}

errcodes motor_goto0(uint8_t i){
    errcodes e = motor_absmove(i, -the_conf.maxsteps[i]);
    if(ERR_OK != e) return e;
    ESW_reaction[i] = ESW_STOPMINUS;
    mvzerostate[i] = M0FAST;
    return e;
}

// smooth motor stopping
void stopmotor(uint8_t i){
    switch(state[i]){
        case STP_MVSLOW: // immeditially stop on slowest speed
            stopflag[i] = 1;
            return;
        break;
        case STP_MOVE:  // stop only in moving states
        case STP_ACCEL:
        break;
        default: // do nothing in other states
            return;
    }
    int32_t newstoppos = stppos[i]; // calculate steps need for stop (we can be @acceleration phase!)
    int32_t add = (curspeed[i] * curspeed[i]) / the_conf.accel[i] / 2;
    if(motdir[i] > 0){
        newstoppos += add;
        if(newstoppos < (int32_t)the_conf.maxsteps[i]) targstppos[i] = newstoppos;
    }else{
        newstoppos -= add;
        if(newstoppos > -((int32_t)the_conf.maxsteps[i])) targstppos[i] = newstoppos;
    }
    TODECEL();
}

void process_steppers(){
    static uint32_t Tlast = 0;
    if(Tms - Tlast < MOTCHKINTERVAL) return; // hit every 10ms
    Tlast = Tms;
    for(int i = 0; i < MOTORSNO; ++i){
        chkstepper(i);
    }
}

uint8_t geteswreact(uint8_t i){
    return ESW_reaction[i];
}
