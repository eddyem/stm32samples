/*
 * This file is part of the ir-allsky project.
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

#include "adc.h"
#include "heater.h"
#include "hardware.h"

#ifdef EBUG
#include "strfunc.h"
#include "usb_dev.h"
#endif

// turn on/off automatic cover heater in case of high humidity
uint8_t AutoHeater = 0;
static float setTemp = 0.f;
// try to hold setTemp
static uint8_t holdTemp = 0;
// no NTC found - errflag
static uint8_t noNTCfound = 0;
// last time env/hold was checked
static uint32_t lastTcheck = 0;

uint8_t getThold(float *T){
    if(T) *T = setTemp;
    return holdTemp;
}

// set Thold in degrC
int setThold(int Th){
    if(Th < -10 || Th > 50) return FALSE;
    if(noNTCfound) return FALSE;
    setTemp = (float) Th;
    holdTemp = 1;
    return TRUE;
}

static void setHeaters(uint8_t PWMval){
    for(uint8_t i = 0; i < HTR_AMOUNT; ++i)
        setPWM(i, PWMval);
}

void clearThold(){
    holdTemp = 0;
    setHeaters(0);
}

// calculate maximal and mean T
// return amount of working NTCs
static int CalcT(float *maxval, float *meanval){
    float max = -100.f, sum = 0.f;
    int N;
    for(uint8_t i = 0; i < NTC_AMOUNT; ++i){
        float Tcur = getNTCtemp(i);
        if(Tcur < -100.f) continue; // bad value: short cirquit or break
        ++N;
        if(Tcur > max) max = Tcur;
        sum += Tcur;
    }
    if(N == 0) return 0; // oops: nothing found
    if(meanval) *meanval = sum / N;
    if(maxval) *maxval = max;
    return N;
}

void heater_process(){
    static uint8_t overheating = 0; // if ==1 wait until Tset to run again
    if(!AutoHeater && !holdTemp) return;
    if(Tms - lastTcheck < HTR_CHECK_PERIOD) return;
    float Tmax, Tmean;
    if(0 == CalcT(&Tmax, &Tmean)){ // turn off heater and return
        setHeaters(0);
        DBG("NTC not found!");
        noNTCfound = 1;
        return;
    }
#ifdef EBUG
    USB_sendstr("Tmax="); USB_sendstr(float2str(Tmax, 1));
    USB_sendstr("\nTmean="); USB_sendstr(float2str(Tmean, 1));
    newline();
#endif
    noNTCfound = 0;
    lastTcheck = Tms;
    if(AutoHeater) do{ // check weather conditions and set/reset holdTemp/setTemp
        bme280_t env;
        if(!get_environment(&env)) break; // can't set holding temperature when dunno env
        if(env.H > HUMIDITY_MAX){
            float Tset = env.T + TDEW_OVER_DELTA + 0.5f;
            if(Tset < TEMP_DEFROST) Tset = TEMP_DEFROST; // defrost
            setThold((int)Tset);
        }else{ // all OK: turn off heaters and clear holding
            setHeaters(0);
            holdTemp = 0;
        }
    }while(0);
    if(holdTemp){ // try to hold `setTemp` +-5 degC
        float Tdiff = Tmean - setTemp; // >0 in case of overheating
        uint8_t PWMset = 0;
        if(Tdiff < -HOLDT_HYSTERESIS){
            PWMset = PWM_START_VAL;
            overheating = 0;
        }else if(!overheating){
            if(Tdiff > HOLDT_HYSTERESIS){
                overheating = 1;
            }else if(Tdiff > 0){
                PWMset = PWM_HOLD_VAL;
            }else PWMset = PWM_MID_VAL;
        }else if(Tdiff < 0.f){ // need slight heating
            PWMset = PWM_HOLD_VAL;
        }
        // check max for overheating
        if(Tmax - setTemp > HOLDT_HYSTERESIS && PWMset > PWM_HOLD_VAL) PWMset = PWM_HOLD_VAL;
#ifdef EBUG
        USB_sendstr("Tdiff="); USB_sendstr(float2str(Tdiff, 1));
        USB_sendstr("\nPWMset="); USB_sendstr(u2str(PWMset));
        newline();
#endif
        setHeaters(PWMset);
    }
}
