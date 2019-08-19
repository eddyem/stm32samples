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
#ifdef EBUG
#include "usart.h"
#endif
#include "usb.h"
#include <string.h>

volatile uint32_t Timer; // milliseconds counter
curtime current_time = TMNOTINI;

static inline uint8_t atou(const char *b){
    return (b[0]-'0')*10 + b[1]-'0';
}

/**
 * @brief set_time - set current time from GPS data
 * @param buf - buffer with time data (HHMMSS)
 */
void set_time(const char *buf){
    uint8_t H = atou(buf);// + TIMEZONE_GMT_PLUS;
    if(H > 23) H -= 24;
    current_time.H = H;
    current_time.M = atou(&buf[2]);
    current_time.S = atou(&buf[4]);
#ifdef EBUG
    SEND("set_time, Tms: "); printu(1, Tms);
    SEND("; Timer: "); printu(1, Timer);
    newline();
#endif
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
#ifdef EBUG
    SEND("time_increment():  ");
    SEND(get_time(&current_time, 0));
#endif
}

static char *puttwo(uint8_t N, char *buf){
    if(N < 10){
        *buf++ = '0';
    }else{
        *buf++ = N/10 + '0';
        N %= 10;
    }
    *buf++ = N + '0';
    return buf;
}

/**
 * @brief ms2str - fill buffer str with milliseconds ms
 * @param str (io) - pointer to buffer
 * @param T - milliseconds
 */
static void ms2str(char **str, uint32_t T){
    char *bptr = *str;
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
    *str = bptr;
}

/**
 * print time: Tm - time structure, T - milliseconds
 */
char *get_time(curtime *Tm, uint32_t T){
    static char buf[64];
    char *bstart = &buf[5], *bptr = bstart;
    int S = 0;
    if(T > 999) return "Wrong time";
    if(Tm->S < 60 && Tm->M < 60 && Tm->H < 24)
        S = Tm->S + Tm->H*3600 + Tm->M*60; // seconds from day beginning
    if(!S) *(--bstart) = '0';
    while(S){
        *(--bstart) = S%10 + '0';
        S /= 10;
    }
    // now bstart is buffer starting index; bptr points to decimal point
    ms2str(&bptr, T);
    // put current time in HH:MM:SS format into buf
    *bptr++ = ' '; *bptr++ = '(';
    bptr = puttwo(Tm->H, bptr); *bptr++ = ':';
    bptr = puttwo(Tm->M, bptr); *bptr++ = ':';
    bptr = puttwo(Tm->S, bptr);
    ms2str(&bptr, T);
    *bptr++ = ')';
    if(GPS_status == GPS_NOTFOUND){
        strcpy(bptr, " GPS not found");
        bptr += 14;
    }
    *bptr = 0;
    return bstart;
}


#ifdef EBUG
int32_t timerval, Tms1;
#endif
int32_t timecntr=0, ticksdiff=0;

uint32_t last_corr_time = 0;

/**
 * @brief systick_correction
 * Makes correction of system timer
 * The default frequency of timer is 1kHz - 72000 clocks per interrupt
 * So we check how much ticks there was for last one second - between PPS interrupts
 * Their amount equal to M = `Timer` value x (SysTick->LOAD+1) + (SysTick->LOAD+1 - SysTick->VAL)
 *    if `Timer` is very small, add 1000 to its value.
 * We need 1000xN ticks instead of M
 * if L = LOAD+1, then
 * M = Timer*L + L - VAL; newL = L + D = M/1000
 * 1000*D = M - 1000*L = L(Timer+1-1000) - VAL ->
 * D = [L*(Timer-999) - VAL]/1000
 * So correction equal to
 *      [ (SysTick->LOAD + 1) * (Timer - 999) - SysTick->VAL ] / 1000
 */
void systick_correction(){
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; // stop systick for a while
    uint32_t systick_val = SysTick->VAL, L = SysTick->LOAD + 1;
    int32_t timer_val = Timer;
#ifdef EBUG
    timerval = Timer;
    Tms1 = Tms;
#endif
    Timer = 0;
    SysTick->VAL = SysTick->LOAD;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; // start it again
//    if(systick_val != SysTick->LOAD) ++Tms;
    if(timer_val > 500) time_increment(); // counter greater than 500 -> need to increment time
    if(last_corr_time){
        if(Tms - last_corr_time < 1500){ // there was perevious PPS signal
            int32_t D = L * (Tms - 1000 - last_corr_time) + (SysTick->LOAD - systick_val); // amount of spare ticks
#ifdef EBUG
            ++timecntr;
#endif
            ticksdiff += D;
            uint32_t ticksabs = (ticksdiff < 0) ? -ticksdiff : ticksdiff;
            // 10000 == 30 seconds * 1000 interrupts per second
            if(ticksabs > 30000 && timecntr > 30){ // need correction (not more often than each 10s)
                ticksdiff /= timecntr * 1000; // correction per one interrupt
                SysTick->LOAD += ticksdiff;
                timecntr = 0;
                ticksdiff = 0;
                last_corr_time = 0;
#ifdef EBUG
                SEND("Correction\n");
#endif
            }
        }else{
            timecntr = 0;
            ticksdiff = 0;
            last_corr_time = 0;
        }
    }
    last_corr_time = Tms;
}

