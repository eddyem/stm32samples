/*
 * This file is part of the Stepper project.
 * Copyright 2020 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "proto.h"
#include "steppers.h"

static drv_type driver = DRV_NONE;

// maximum number of microsteps per driver
static const uint16_t maxusteps[] = {
    [DRV_NONE] = 0,
    [DRV_NOTINIT] = 0,
    [DRV_MAILF] = 0,
    [DRV_8825] = 32,
    [DRV_4988] = 16,
    [DRV_2130] = 256
};

// amount of steps need for full acceleration/deceleration cycle
#define ACCDECSTEPS (the_conf.accdecsteps)
// amount of microsteps in each step
#define USTEPS      (the_conf.microsteps)

int32_t mot_position = -1;  // current position of motor (from zero endswitch, -1 means inactive)
uint32_t steps_left = 0;    // amount of steps left
stp_state state = STP_SLEEP;// current state of motor
// ARR register values: low (max speed), high (min speed = 10% from max), step (1/50(hi-lo))
static uint16_t stplowarr, stphigharr, stpsteparr;
static int8_t dir = 0; // moving direction: -1 (negative) or 1 (positive)

/**
 * @brief checkDrv - test if driver connected
 */
static void checkDrv(){
    uint8_t oldstate;
    // turn power on and wait a little
    SLEEP_ON(); // sleep -> 0
    DRV_DISABLE();
    VIO_ON(); sleep(15);
    // check Vdd
    if(FAULT_STATE()){
        MSG("No power @ Vin, mailfunction\n");
        driver = DRV_MAILF;
        VIO_OFF();
        goto ret;
    }
    SLP_CFG_IN();
    sleep(2);
    // Check is ~SLEEP is in air
    oldstate = SLP_STATE();
#ifdef EBUG
    if(oldstate) MSG("SLP=1\n"); else MSG("SLP=0\n");
#endif
    SLEEP_OFF(); SLP_CFG_OUT();  // sleep -> 1
    sleep(2);
    SLP_CFG_IN();
    sleep(2);
#ifdef EBUG
    if(SLP_STATE()) MSG("SLP=1\n"); else MSG("SLP=0\n");
#endif
    if(SLP_STATE() != oldstate){
        MSG("~SLP is in air\n");
        if(driver != DRV_2130){
            driver = DRV_NONE;
            VIO_OFF();
        }
        goto ret;
    }
    SLEEP_ON(); SLP_CFG_OUT();
    // check if ~EN is in air
    DRV_ENABLE(); // EN->0
    sleep(2);
    EN_CFG_IN();
    sleep(2);
    oldstate = EN_STATE();
#ifdef EBUG
    if(oldstate) MSG("EN=1\n"); else MSG("EN=0\n");
#endif
    DRV_DISABLE(); // EN->1
    EN_CFG_OUT();
    sleep(2);
    EN_CFG_IN();
    sleep(2);
#ifdef EBUG
    if(EN_STATE()) MSG("EN=1\n"); else MSG("EN=0\n");
#endif
    if(oldstate != EN_STATE()){
        MSG("~EN is in air\n");
        driver = DRV_NONE;
        VIO_OFF();
        goto ret;
    }
ret:
    DRV_DISABLE();
    EN_CFG_OUT(); // return OUT conf
    SLEEP_OFF();
    SLP_CFG_OUT();
#ifdef EBUG
    sendbuf();
#endif
}

static drv_type ini2130(){ // init 2130: SPI etc.
    if(driver != DRV_2130) return DRV_MAILF;
    ;
    return DRV_MAILF;
}

static drv_type ini4988_8825(){ // init 4988 or 8825
    if(driver != DRV_4988 && driver != DRV_8825){
        MSG("Wrong drv\n");
        return DRV_MAILF;
    }
    if(the_conf.microsteps > maxusteps[driver]){
        SEND("Wrong microstepping settings\n");
        return DRV_MAILF;
    }
    if(the_conf.microsteps == 0){
        SEND("Configure microstepping first\n");
        return DRV_MAILF;
    }
    // init microstepping pins and set config
    UST01_CFG_PP();
    uint8_t PINS = 0;
    if(the_conf.microsteps == 16 && driver == DRV_4988) PINS = 7; // microstepping settings for 4988 in 1/16 differs from 8825
    else PINS = (uint8_t)__builtin_ctz(the_conf.microsteps);
#ifdef EBUG
    SEND("Microstep PINS=");
    printu(PINS);
    newline(); sendbuf();
#endif
    // now PINS is M0..M2 settings
    if(PINS & 1) SET_UST0(); else RESET_UST0();
    if(PINS & 2) SET_UST1(); else RESET_UST1();
    if(PINS & 4) SET_UST2(); else RESET_UST2();
    // turn on timer
    timer_setup();
    // recalculate defaults
    stp_chspd();
    SEND("Init OK\n");
    return driver;
}

/**
 * @brief initDriver - try to init driver
 * @return driver type
 */
drv_type initDriver(){
    if(driver != DRV_NOTINIT){ // reset all settings
        MSG("clear GPIO & other setup\n");
        STEP_TIMER_OFF();
        // TODO: turn off SPI & timer
        gpio_setup(); // reset pins control
    }
    driver = the_conf.driver_type;
    checkDrv();
    if(driver > DRV_MAX-1) return (driver = DRV_NONE);
    MSG("init pins\n");
    switch(driver){
        case DRV_2130:
            return ini2130();
        break;
        case DRV_4988:
        case DRV_8825:
            return ini4988_8825();
        break;
        default:
            SEND("Set driver type in config\n");
            return driver; // bad driver type
    }
    return driver;
}

uint16_t getMaxUsteps(){
    if(driver > DRV_MAX-1) return 0;
    return maxusteps[driver];
}

void stp_chspd(){
    int i;
    for(i = 0; i < 2; ++i){
        uint16_t spd = the_conf.motspd;
        stplowarr = spd;
        stphigharr = spd * LOWEST_SPEED_DIV;
        stpsteparr = (spd * (LOWEST_SPEED_DIV - 1)) / ((uint16_t)ACCDECSTEPS) + 1;
    }
}

// check end-switches for stepper motors
void stp_process(){
    // check end-switches; ESW0&ESW3 stops motor
    uint8_t esw = ESW_STATE();
    switch(state){
        case STP_MOVE0: // move towards ESW0
            state = STP_SLEEP;
            stp_move(-the_conf.maxsteps); // won't move if the_conf.maxsteps == 0
        break;
        case STP_MOVE1: // move towards ESW3
            state = STP_SLEEP;
            stp_move(the_conf.maxsteps);
        break;
        case STP_ACCEL: // @ any move check esw
        case STP_DECEL:
        case STP_MOVE:
        case STP_MVSLOW:
            if((esw&1) && dir == -1){ // move through ESW0
                state = STP_STOPZERO; // stop @ end-switch
            }else if((esw&8) && dir ==  1){ // move through ESW3
                state = STP_STOP; // stop @ ESW3
            }
        break;
        default: // stopping states - do nothing
        break;
    }
}

// move motor to `steps` steps, @return 0 if all OK
stp_status stp_move(int32_t steps){
    if(state != STP_SLEEP && state != STP_MOVE0 && state != STP_MOVE1) return STPS_ACTIVE;
    if(steps == 0)
        return STPS_ZEROMOVE;
    if(the_conf.maxsteps && steps > (int32_t)the_conf.maxsteps) return STPS_TOOBIG;
    int8_t d;
    if(steps < 0){
        d = -1;
        steps = -steps;
    }else d = 1; // positive direction
    // check end-switches
    uint8_t esw = ESW_STATE();
    if(((esw&1) && d == -1) || ((esw&8) && d == 1)) return STPS_ONESW; // can't move through esw
    dir = d;
    // change value of DIR pin
    if(the_conf.defflags.reverse){
        if(d>0)
            SET_DIR();
        else
            CLEAR_DIR();
    }else{
        if(d>0)
            CLEAR_DIR();
        else
            SET_DIR();
    }
    // turn on driver, EN=0
    DRV_ENABLE();
    steps_left = (uint32_t)steps;
    // setup timer & start it
    TIMx->ARR = stphigharr;
    TIMx->CCMR1 = TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1; // PWM mode 1: active->inacive, preload enable
    TIMx->CR1 |= TIM_CR1_CEN;
    if(steps < ACCDECSTEPS*2) state = STP_MVSLOW; // move without acceleration
    else state = STP_ACCEL; // move with acceleration
    return STPS_ALLOK;
}

// change ARR value
void stp_chARR(uint32_t val){
    if(val < 2) val = 2;
    TIMx->ARR = (uint32_t)val;
}

void stp_stop(){ // stop motor by demand or @ end-switch
    switch(state){
        case STP_SLEEP:
            return;
        break;
        case STP_MOVE0:
        case STP_MOVE1:
            state = STP_SLEEP;
        break;
        default:
            state = STP_STOP;
    }
}

void timer_isr(){
    static uint16_t ustep = 0;
    uint16_t tmp, arrval;
    if(USTEPS == ++ustep){ // prevent stop @ not full step
        ustep = 0;
        if(state == STP_STOPZERO)
            mot_position = 0;
        else{
            if(0 == --steps_left) state = STP_STOP;
            mot_position += dir;
        }
    }else return;
    switch(state){
        case STP_ACCEL: // acceleration phase
            arrval = (uint16_t)TIMx->ARR - stpsteparr;
            tmp = stplowarr;
            if(arrval <= tmp || arrval > stphigharr){
                arrval = tmp;
                state = STP_MOVE; // end of acceleration phase
            }
            TIMx->ARR = arrval;
        break;
        case STP_DECEL: // deceleration phase
            arrval = (uint16_t)TIMx->ARR + stpsteparr;
            tmp = stphigharr;
            if(arrval >= tmp || arrval < stplowarr){
                arrval = tmp;
                state = STP_MVSLOW; // end of deceleration phase, move @ lowest speed
            }
            TIMx->ARR = arrval;
        break;
        case STP_MOVE:   // moving with constant speed phases
            if(steps_left <= ACCDECSTEPS){
                state = STP_DECEL; // change moving status to decelerate
            }
        break;
        case STP_MVSLOW:
            // nothing to do here: all done before switch()
        break;
        default: // STP_STOP, STP_STOPZERO
            ustep = 0;
            TIMx->CCMR1 = TIM_CCMR1_OC1M_2; // Force inactive
            TIMx->CR1 &= ~TIM_CR1_CEN; // stop timer
            DRV_DISABLE();
            dir = 0;
            steps_left = 0;
            state = STP_SLEEP;
        break;
    }
    TIMx->SR = 0;
}
