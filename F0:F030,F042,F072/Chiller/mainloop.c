/*
 * This file is part of the Chiller project.
 * Copyright 2019 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "mainloop.h"
#include "hardware.h"
#include "adc.h"

int16_t Tset = 200; // temperature setpoint
int16_t NTCval[4] = {0,};

// common status for all functions from this file; pointer to this variable return @mainloop
static chiller_state retstatus = {
    .common_state = ST_OK,
    .heater_state = ST_OK,
    .cooler_state = ST_OK,
    .pump_state   = ST_OK
};

/* error bit fields: */
// chiller_error==0 - no errors
#define CE_NOERROR      (0)
// heater error (overheating)
#define CE_HEATER       (1<<0)
// output was too hot
#define CE_OUTHOT       (1<<1)
// output was too cool
#define CE_OUTCOOL      (1<<2)
// no flow sensor pulses detected
#define CE_NOFLOW       (1<<3)

// error code explaining why alarm is working
static uint8_t chiller_error;

static inline void increase_pump_pwm(){
    uint16_t pwm = GET_PUMP_PWM();
    if(pwm < 246){
        SET_PUMP_PWM(pwm+10);
        retstatus.pump_state = ST_FASTER;
    }
}
static inline void decrease_pump_pwm(){
    uint16_t pwm = GET_PUMP_PWM();
    if(pwm > MIN_PUMP_PWM+9){
        SET_PUMP_PWM(pwm-10);
        retstatus.pump_state = 0; // "ST_SLOWER"
    }
}

/**
 * @brief binsrch - binary search for new good value
 * @param oldval  - previuos value
 * @param curval  - current value
 * @param dir     - direction (1 - increase, 0 - decrease)
 * @return new value
 */
static inline uint16_t binsrch(uint16_t oldval, uint16_t curval, uint8_t dir){
    if(oldval == curval){
        if(dir) oldval = 256;
        else oldval = 0;
    }else{
        if(dir){ // increase
            if(oldval == 0) oldval = 256;
            else if(oldval < curval){
                oldval = 2*curval - oldval;
            }
        }else{   // decrease
            if(curval == 0) oldval = 0;
            else if(oldval > curval){
                oldval = 2*curval - oldval;
            }
        }
    }
    oldval = (oldval + curval) / 2;
    if(oldval > 255) oldval = 0;
    return oldval;
}

// change PWM according to dir (1->up, 0->down)
static void change_heater_pwm(uint8_t dir){
    static uint16_t oldpwm = 0;
    uint16_t pwm = binsrch(oldpwm, GET_HEATER_PWM(), dir);
    if(pwm != GET_HEATER_PWM()){
        oldpwm = GET_HEATER_PWM();
        SET_HEATER_PWM(pwm);
        if(dir){ // up
            retstatus.heater_state = ST_FASTER;
        }else{ // down
            if(pwm == 0) retstatus.heater_state = ST_OFF;
            else retstatus.heater_state = 0; // "ST_SLOWER"
        }
    }
}
static void change_cooler_pwm(uint8_t dir){
    uint16_t pwm = GET_COOLER_PWM();
    if(dir){ // up
        if(pwm < 224) SET_COOLER_PWM(pwm + 32);
        else SET_COOLER_PWM(255);
        if(pwm != GET_COOLER_PWM())
            retstatus.cooler_state = ST_FASTER;
    }else{ // down
        if(pwm > MIN_COOLER_PWM + 31) SET_COOLER_PWM(pwm - 32);
        else SET_COOLER_PWM(0);
        if(pwm != GET_COOLER_PWM())
            retstatus.cooler_state = GET_COOLER_PWM() ? 0 : ST_OFF; // "ST_SLOWER" / ST_OFF
    }
}

/**
 * @brief get_critical - check device for critical errors
 * @return 1 if critical error occured
 */
static inline uint8_t get_critical(){
    uint8_t ret = 0;
    // critical state: heater can burn out!
    // turn off heater & make signal
    if(HEATER_TEMPERATURE > MAX_HEATER_T){
        // change heater state to CRIT_HOFF
        retstatus.heater_state = ST_CRITICAL;
        if(GET_HEATER_PWM()){
            SET_HEATER_PWM(0);
            retstatus.heater_state |= ST_OFF;
        }
        chiller_error |= CE_HEATER;
        ret = 1;
    }
    // very hot output: turn off heater, turn on cooler & make signal
    if(OUTPUT_TEMPERATURE > MAX_OUTPUT_T){
        // change chiller state to CRIT_HOT
        retstatus.common_state = ST_CRITICAL;
        if(GET_HEATER_PWM()){
            SET_HEATER_PWM(0);
            retstatus.heater_state = ST_OFF;
        }
        if(GET_COOLER_PWM() < 255){
            SET_COOLER_PWM(255);
            retstatus.cooler_state = ST_FASTER;
        }
        // if water @input is also too hot, turn pump to max speed
        if(INPUT_TEMPERATURE > MAX_OUTPUT_T){
            increase_pump_pwm();
        }
        chiller_error |= CE_OUTHOT;
        ret = 1;
    }
    // very cool output: turn on heater (max power), turn off cooler & make signal
    if(OUTPUT_TEMPERATURE < MIN_OUTPUT_T){
        retstatus.common_state = ST_CRITICAL|ST_FASTER;
        if(GET_HEATER_PWM() < 255){
            SET_HEATER_PWM(255);
            retstatus.heater_state = ST_FASTER;
        }
        if(GET_COOLER_PWM()){
            SET_COOLER_PWM(0);
            retstatus.cooler_state = ST_OFF;
        }
        chiller_error |= CE_OUTCOOL;
        ret = 1;
    }
    // check flow rate & pump working
    if(GET_PUMP_PWM() >= MIN_PUMP_PWM){ // pump working
        if(flow_rate < MIN_FLOW_RATE){ // check pump
            // change chiller state to CRIT_NOFLOW
            retstatus.common_state = ST_CRITICAL|ST_OK;
            // increase pump speed
            //increase_pump_pwm();
            retstatus.pump_state = ST_CRITICAL;
            chiller_error |= CE_NOFLOW;
            ret = 1;
        }
    }else{
        // turn ON pump if PWM < minimal
        // (pump should be never off!)
        SET_PUMP_PWM(MIN_PUMP_PWM);
        retstatus.pump_state = ST_FASTER;
    }
    return ret;
}

/**
 * @brief check_alarm - check device status and turn off alarm if it is on
 */
static inline void check_alarm(){
    if(!ALARM_STATE()) return;
    // check errors & turn alarm OFF if there's no critical situations
    if(chiller_error == CE_NOERROR){
        // turn off alarm if there's no more errors
        ALARM_OFF();
    }else{
        if(chiller_error & CE_HEATER){ // clear CE_HEATER if heater T is normal
            if(HEATER_TEMPERATURE < NORMAL_HEATER_T){
                chiller_error &= ~CE_HEATER;
            }
        }
        if(chiller_error & CE_OUTHOT){ // clear CE_OUTHOT if Tout is normal
            if(OUTPUT_TEMPERATURE < OUTPUT_T_H){
                chiller_error &= ~CE_OUTHOT;
            }
        }
        if(chiller_error & CE_OUTCOOL){ // clear CE_OUTCOOL if Tout is normal
            if(OUTPUT_TEMPERATURE > OUTPUT_T_L){
                chiller_error &= ~CE_OUTCOOL;
            }
        }
        if(chiller_error & CE_NOFLOW){ // clear CE_NOFLOW if there's flow pulses
            if(flow_rate > NORMAL_FLOW_RATE){
                chiller_error &= ~CE_NOFLOW;
            }
        }
    }
}

static inline void checkOutT(){
    // check that T is between limits
    int8_t hc = 0; // need heating or cooling?
    if(OUTPUT_TEMPERATURE > Tset + TEMP_TOLERANCE) hc = -1; // need cooling
    else if(OUTPUT_TEMPERATURE < Tset - TEMP_TOLERANCE) hc = 1; // need heating
    if(hc){// out of limits -> check
        if(hc > 0){ // need heating: turn off cooler & turn on heater
            if(GET_COOLER_PWM()){
                SET_COOLER_PWM(0);
                retstatus.cooler_state = ST_OFF;
            }
            if(GET_HEATER_PWM() < 255){
                SET_HEATER_PWM(255);
                retstatus.heater_state = ST_FASTER;
            }else{
                // bad situation: need MORE heating!
            }
        }else{ // need cooling: turn off heater & turn on cooler
            if(GET_HEATER_PWM()){
                SET_HEATER_PWM(0);
                retstatus.heater_state = ST_OFF;
            }
            if(GET_COOLER_PWM() < 255){
                SET_COOLER_PWM(255);
                retstatus.cooler_state = ST_FASTER;
            }else{
                // bad situation: need MORE cooling!
            }
        }
    }else{ // T inside borders -> correct heating/cooling speed
        // Tout > Tset -> heater PWM up & cooler PWM down
        // else -> vice versa
        uint8_t ht = 2; // don't need heater/cooler change
        if(OUTPUT_TEMPERATURE < Tset - DT_TOLERANCE) ht = 1; // need heating
        else if(OUTPUT_TEMPERATURE > Tset + DT_TOLERANCE) ht = 0; // need cooling
        if(ht != 2){
            change_heater_pwm(ht);
            change_cooler_pwm(!ht);
        }
    }
    // if all OK, make pump slower
    if(retstatus.pump_state == ST_OK){
        decrease_pump_pwm();
    }
}

/**
 * @brief mainloop - the main chiller loop
 * by timer check current states & change them
 */
chiller_state *mainloop(){
    static uint32_t lastTmeas = 0xffff; // Temperatures measurement time
    static uint32_t lastTchk = 0xffff;  // last state checking time
    retstatus.common_state = ST_OK;
    retstatus.heater_state = ST_OK;
    retstatus.cooler_state = ST_OK;
    retstatus.pump_state   = ST_OK;
    // 1. Get temperatures and check critical situations
    if(Tms - lastTmeas < TMEASURE_MS) return &retstatus;
    lastTmeas = Tms;
    for(int i = 0; i < 4; ++i) // refresh NTC values
        NTCval[i] = getNTC(i);
    uint8_t alrm = get_critical();
    // check cooler
    if(GET_COOLER_PWM() > MIN_COOLER_PWM){ // cooler working
        // air temperature is very hot - cooler useless
        if(AIR_TEMPERATURE > OUTPUT_TEMPERATURE + TEMP_TOLERANCE){
            // change cooler state to OFF
            if(GET_COOLER_PWM()){
                SET_COOLER_PWM(0);
                retstatus.cooler_state = ST_OFF;
            }
        }
    }else{
        if(GET_COOLER_PWM()){
            SET_COOLER_PWM(0);
            retstatus.cooler_state = ST_OFF;
        }
    }
    // check alarm
    if(alrm){
        ALARM_ON();
        return &retstatus;
    }
    // there wasn't critical cases in this iteration, go further
    check_alarm();
    // Now check thermal data and decide what to do
    if(Tms - lastTchk < TCHECK_MS) return &retstatus;
    lastTchk = Tms;
    checkOutT();
    return &retstatus;
}

