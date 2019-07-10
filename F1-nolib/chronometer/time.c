/*
 * This file is part of the chronometer project.
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

#include "GPS.h"
#include "time.h"
#include "usb.h"
#include <string.h>

volatile uint32_t Timer; // milliseconds counter
curtime current_time = TMNOTINI;
volatile int need_sync = 1;

// SysTick->LOAD values for all milliseconds (RVR0) and last millisecond (RVR1)
static uint32_t RVR0 = SYSTICK_DEFLOAD, RVR1 = SYSTICK_DEFLOAD;

static inline uint8_t atou(const char *b){
    return (b[0]-'0')*10 + b[1]-'0';
}

void set_time(const char *buf){
    uint8_t H = atou(buf) + TIMEZONE_GMT_PLUS;
    if(H > 23) H -= 24;
    current_time.H = H;
    current_time.M = atou(&buf[2]);
    current_time.S = atou(&buf[4]);
}

/**
 * @brief time_increment - increment system timer by systick
 */
void time_increment(){
    Timer = 0;
    if(current_time.H == 25) return; // Time not initialized
    if(++current_time.S == 60){
        current_time.S = 0;
        if(++current_time.M == 60){
            current_time.M = 0;
            if(++current_time.H == 24)
                current_time.H = 0;
        }
    }
}

/**
 * print time: Tm - time structure, T - milliseconds
 */
char *get_time(curtime *Tm, uint32_t T){
    static char buf[64];
    char *bstart = &buf[5], *bptr = bstart;
    /*
    void putint(int i){ // put integer from 0 to 99 into buffer with leading zeros
        if(i > 9){
            *bptr++ = i/10 + '0';
            i = i%10;
        }else *bptr++ = '0';
        *bptr++ = i + '0';
    }*/
    int S = 0;
    if(Tm->S < 60 && Tm->M < 60 && Tm->H < 24)
        S = Tm->S + Tm->H*3600 + Tm->M*60; // seconds from day beginning
    if(!S) *(--bstart) = '0';
    while(S){
        *(--bstart) = S%10 + '0';
        S /= 10;
    }
    // now bstart is buffer starting index; bptr points to decimal point
    *bptr++ = '.';
    if(T > 99){
        *bptr++ = T/100 + '0';
        T %= 100;
    }else *bptr++ = '0';
    if(T > 9){
        *bptr++ = T/10 + '0';
        T %= 10;
    }else *bptr++ = '0';
    *bptr++ = T + '0';
    if(GPS_status == GPS_NOT_VALID){
        strcpy(bptr, " (not valid)");
        bptr += 12;
    }
    if(need_sync){
        strcpy(bptr, " need synchronisation");
        bptr += 21;
    }
    *bptr++ = '\n';
    *bptr = 0;
    return bstart;
}

/**
 * @brief get_millis - calculate milliseconds due to global fix parameter
 * @return milliseconds value
 */
uint32_t get_millis(){
    // TODO: calculate right millis
    return Tms % 1000; // temporary gag
}

void systick_correction(){
    uint32_t t = 0, ticks;
    static uint32_t ticksavr = 0, N = 0, last_corr_time = 0;
    // correct
    int32_t systick_val = SysTick->VAL;
    // SysTick->LOAD values for all milliseconds (RVR0) and last millisecond (RVR1)
    SysTick->VAL = RVR0;
    int32_t timer_val = Timer;
    Timer = 0;
    // RVR -> SysTick->LOAD
    systick_val = SysTick->LOAD + 1 - systick_val; // Systick counts down!
    if(timer_val < 10) timer_val += 1000; // our closks go faster than real
    else if(timer_val < 990){ // something wrong
        RVR0 = RVR1 = SYSTICK_DEFLOAD;
        SysTick->LOAD = RVR0;
        need_sync = 1;
        goto theend;
    }else
        time_increment(); // ms counter less than 1000 - we need to increment time
    t = current_time.H * 3600 + current_time.M * 60 + current_time.S;
    if(t - last_corr_time == 1){ // PPS interval == 1s
        ticks = systick_val + (timer_val-1)*(RVR0 + 1) + RVR1 + 1;
        ++N;
        ticksavr += ticks;
        if(N > 20){
            ticks = ticksavr / N;
            RVR0 = ticks / 1000 - 1; // main RVR value
            SysTick->LOAD = RVR0;
            RVR1 = RVR0 + ticks % 1000; // last millisecond RVR value (with fine correction)
            N = 0;
            ticksavr = 0;
            need_sync = 0;
        }
    }else{
        N = 0;
        ticksavr = 0;
    }
theend:
    last_corr_time = t;
}

void increment_timer(){
    ++Timer;
    if(Timer == 999){
        SysTick->LOAD = RVR1;
    }else if(Timer == 1000){
        SysTick->LOAD = RVR0;
        time_increment();
    }
}
