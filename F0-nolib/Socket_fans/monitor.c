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
#include "monitor.h"
#include "proto.h"

// when critical T reached wait for TturnOff ms and after that turn off system
#define TturnOff    (20000)
// don't mind when button 2 pressed again after t<5s
#define TbtnPressed (5000)

// settings
// T0 - CPU, T1 - HDD, T2 - inner T, T3 - power source
static const int16_t Thysteresis = 30; // hysteresis by T=3degC
static const int16_t tmin[3] = {400, 350, 350}; // turn off fans when T[x]<tmin[x]+Th, turn on when >tmin+Th
static const int16_t tmax[3] = {900, 800, 600}; // critical T, turn off power after TturnOff milliseconds
static const int16_t t3max = 850;

static uint8_t dontprocess = 0; // don't process monitor

static uint32_t TOff = 0; // time to turn off power
static void chkOffRelay(){
    if(Tms > TOff){
        TOff = 0;
        OFF(RELAY);
        SEND("Turn off power");
        buzzer = BUZZER_OFF;
    }else SEND("TCRITSHUTDOWNn");
    NL();
}
static void startOff(){
    SEND("TCRITSHUTDOWN"); NL();
    TOff = Tms + TturnOff;
    if(TOff == 0) TOff = 1;
    buzzer = BUZZER_LONG;
}

// check buttons state
static void chkButtons(){
    static uint32_t Tpressed = 0;
    if(CHK(BUTTON0) == 0){ // button 0 pressed - turn on power if was off
        if(CHK(RELAY) == 0){
            ON(RELAY);
            SEND("Button0: relay ON");NL();
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
         SEND("Everything is OFF immediately");NL();
    }else Tpressed = 0;
}

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
            if(TOff){
                chkOffRelay();
            }else{
                offreason[x] = 1;
                startOff();
                *ccr = 100;
            }
        }else{
            offreason[x] = 0;
            // check working fan
            int chk = (x==0) ? CHK(COOLER0) : CHK(COOLER1);
            if(chk){ // fan is working, check RPM
                if(RPM && speed == 0){
SEND("RPM: "); printu(RPM); SEND(", speed: "); printu(speed); newline();
                    SEND("Cooler"); bufputchar('0'+x); SEND(" died!");NL();
                    buzzer = BUZZER_SHORT;
                    coolerproblem[x] = 1;
                }else if(coolerproblem[x]){
                    buzzer = BUZZER_OFF;
                    coolerproblem[x] = 0;
                    SEND("Cooler OK");NL();
                }
            }else{ // turn on fan
                SEND("Turn fan ON"); NL();
                if(x==0) ON(COOLER0);
                else ON(COOLER1);
            }
            if(T > tmin[x] && T < tmax[x]){ // T between tmin and tmax
                int32_t tx = 80*(T - tmin[x]);
                tx /= (tmax[x] - tmin[x]);
                uint32_t pwm = 20 + tx;
                if(pwm < 20) pwm = 20;
                else if(pwm > 100) pwm = 100;
SEND("Set pwm"); bufputchar('0'+x);SEND(" to "); printu(pwm);NL();
                *ccr = pwm;
            }
        }
    }
    // T2 - not controlled cooler
    T = getNTC(2);
    if(T < tmin[2] - Thysteresis){ // turn off fan
        TIM1->CCR3 = 0;
    }else if(T > tmax[2] + Thysteresis){
        if(TOff){
            chkOffRelay();
        }else{
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
SEND("Set pwm3 to "); printu(pwm);NL();
            TIM1->CCR3 = pwm;
        }
    }
    // T3 - turn off power after TturnOff
    if(getNTC(3) > t3max){
        if(TOff){
            chkOffRelay();
        }else{
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
            SEND("No reason to panic");NL();
            TOff = 0;
            buzzer = BUZZER_OFF;
        }
    }
}
