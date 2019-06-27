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

curtime current_time = TMNOTINI;
volatile int need_sync = 1;

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
