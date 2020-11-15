/*
 * This file is part of the SockFans project.
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

#include "adc.h"
#include "flash.h"
#include "monitor.h"
#include "proto.h"

// when critical T reached wait for TturnOff ms and after that turn off system
#define TturnOff    the_conf.Tturnoff
// don't mind when button 2 pressed again after t<5s
#define TbtnPressed (5000)

// settings
// T0 - CPU, T1 - HDD, T2 - inner T, T3 - power source
#define Thysteresis (int16_t)the_conf.Thyst
#define tmin        the_conf.Tmin
#define tmax        the_conf.Tmax
#define t3max       the_conf.Tmax[TMAXNO-1]

static uint8_t dontprocess = 0; // don't process monitor
static uint32_t TOff = 0; // time to turn off power

static void chkOffRelay(){
    static uint32_t scntr = 0;
    if(!TOff){ // warning cleared
        scntr = 0;
        return;
    }
    if(Tms > TOff){
        TOff = 0;
        OFF(RELAY);
        //TIM1->CCR1 = 0; TIM1->CCR2 = 0; TIM1->CCR3 = 0;
        //dontprocess = 1;
        SEND("POWEROFF");
        buzzer = BUZZER_OFF;
        scntr = 0;
    }else{
        SEND("TCRITSHUTDOWN");
        printu(++scntr);
    }
    NL();
}
static void startOff(){
    SEND("TCRITSHUTDOWN0"); NL();
    TOff = Tms + TturnOff;
    if(TOff == 0) TOff = 1;
    buzzer = BUZZER_ON;
}

void SetDontProcess(uint8_t newstate){ // set dontprocess value
    dontprocess = newstate;
}
uint8_t GetDontProcess(){ // get dontprocess value
    return dontprocess;
}


// check buttons state
static void chkButtons(){
    static uint32_t Tpressed = 0;
    if(CHK(BUTTON0) == 0){ // button 0 pressed - turn on power if was off
        if(CHK(RELAY) == 0){
            ON(RELAY);
            SEND("POWERON"); NL();
        }
        dontprocess = 0;
    }
    if(CHK(BUTTON1) == 0){ // button 1 - all OFF
         if(Tpressed){ // already pressed recently
             if(Tms - Tpressed < TbtnPressed) return;
         }
         Tpressed = Tms;
         if(Tpressed == 0) Tpressed = 1;
         buzzer = BUZZER_OFF;
         TIM1->CCR1 = 0; TIM1->CCR2 = 0; TIM1->CCR3 = 0;
         OFF(RELAY);
         dontprocess = 1;
         SEND("POWEROFF"); NL();
    }else Tpressed = 0;
}

// maximum value of coolerproblem counter
#define MAXCOOLERPROBLEM        (10)

// monitor temperatures and do something
void process_monitor(){
    chkButtons();
    if(dontprocess) return;
    int16_t T;
    static uint8_t coolerproblem[2] = {0,0}; // cooler don't run
    static uint8_t offreason[4] = {0}; // whos T was critical for turning power off
    // check all 4 temperatures
    // T0..1 - coolers
    for(int x = 0; x < 2; ++x){
        T = getNTC(x);
        volatile uint32_t *ccr = (x == 0) ? &TIM1->CCR1 : &TIM1->CCR2;
        uint32_t RPM = *ccr;
        uint32_t speed = Coolerspeed[x];
        if(T < tmin[x] - Thysteresis){ // turn off fan
            *ccr = 0;
        }else if(T > tmax[x] + Thysteresis){
            gett('0'+x); NL();
            if(!TOff){
                offreason[x] = 1;
                startOff();
                *ccr = 100;
            }
        }else{
            offreason[x] = 0;
            // check working fan
            int chk = (x==0) ? CHK(COOLER0) : CHK(COOLER1);
            if(chk){ // fan is working, check RPM
                if(RPM && (speed == 0)){
                    if(coolerproblem[x] > MAXCOOLERPROBLEM){
                        buzzer = BUZZER_SHORT;
                        SEND("COOLER"); bufputchar('0'+x); SEND("DEAD");NL();
                    }else ++coolerproblem[x];
                }else if(coolerproblem[x]){
                    buzzer = BUZZER_OFF;
                    coolerproblem[x] = 0;
                    SEND("COOLER"); bufputchar('0'+x); SEND("OK");NL();
                }
            }else{ // turn on fan
                if(x==0) ON(COOLER0);
                else ON(COOLER1);
            }
            if(T > tmin[x] && T < tmax[x]){ // T between tmin and tmax
                int32_t tx = 80*(T - tmin[x]);
                tx /= (tmax[x] - tmin[x]);
                uint32_t pwm = 20 + tx;
                if(pwm < 20) pwm = 20;
                else if(pwm > 100) pwm = 100;
                *ccr = pwm;
            }
        }
    }
    // T2 - not controlled cooler
    T = getNTC(2);
    if(T < tmin[2] - Thysteresis){ // turn off fan
        TIM1->CCR3 = 0;
    }else if(T > tmax[2] + Thysteresis){
        gett('2'); NL();
        if(!TOff){
            offreason[2] = 1;
            startOff();
            TIM1->CCR3 = 100;
        }
    }else{ // T between tmin and tmax
        offreason[2] = 0;
        if(T > tmin[2] && T < tmax[2]){
            int32_t tx = 60*(T - tmin[2]);
            tx /= (tmax[2] - tmin[2]);
            uint32_t pwm = 40 + tx;
            if(pwm < 40) pwm = 40;
            else if(pwm > 100) pwm = 100;
            TIM1->CCR3 = pwm;
        }
    }
    // T3 - turn off power after TturnOff
    if(getNTC(3) > t3max){
        gett('3'); NL();
        // all coolers to max value
        TIM1->CCR1 = 100;
        TIM1->CCR2 = 100;
        TIM1->CCR3 = 100;
        ON(COOLER0); ON(COOLER1);
        if(!TOff){
            offreason[3] = 1;
            startOff();
        }
    }else offreason[3] = 0;
    // check offreason
    if(TOff){
        uint8_t stillbad = 0;
        for(int x = 0; x < 4; ++x)
            if(offreason[x]){
                stillbad = 1;
                break;
            }
        if(!stillbad){
            SEND("CLRPANIC");NL();
            TOff = 0;
            buzzer = BUZZER_OFF;
        }
        chkOffRelay();
    }
}
