/*
 * This file is part of the rtc project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32g0.h>
#include <string.h>

#include "rtc.h"
#include "strfunc.h"
#include "usart.h"

extern volatile uint32_t Tms;

const char *helpstring =
        "C - set/print calibration ticks (-511..+512) to each 2^20 ticks\n"
        "Sd - set date (YY MM DD Weekday)\n"
        "St - set time (HH MM SS)\n"
        "t - print current Tms\n"
        "T - print current Time\n"
;

TRUE_INLINE void putch(char x){
    usart3_send(&x, 1);
}

static const char *weekdays[] = {"Mon", "Tue", "Wed", "Thur", "Fri", "Sat", "Sun"};
static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sept", "Oct", "Nov", "Dec"};
static const char *OK = "OK\n";

static void prezero(uint8_t x){
    if(x < 10){ putch('0'); putch('0' + x);}
    else USND(u2str(x));
}

void print_curtime(){
    rtc_t t;
    get_curtime(&t);
    USND(weekdays[t.weekday - 1]);
    putch(' ');
    USND(months[t.month - 1]);
    putch(' ');
    USND(u2str(t.day));
    putch(' ');
    prezero(t.hour);
    putch(':');
    prezero(t.minute);
    putch(':');
    prezero(t.second);
    USND(" 20"); prezero(t.year);
    putch('\n');
}

static int readNu8(const char *buf, uint8_t *arr, int maxlen){
    uint32_t D;
    const char *nxt;
    int N = 0;
    while((nxt = getnum(buf, &D)) && nxt != buf && N < maxlen){
        buf = nxt;
        if(D > 0xff){
            USND("Value too large\n");
            return N;
        }
        arr[N++] = (uint8_t) D&0xff;
    }
    return N;
}

TRUE_INLINE const char* setdatetime(const char *buf){
    buf = omit_spaces(buf);
    uint8_t array[4];
    rtc_t r;
    switch(*buf){
        case 'd': // set date
            if(4 != readNu8(buf+1, array, 4))
                return "Format: YY MM DD Weekday (monday is 1)\n";
            r.year = array[0]; 
            r.month = array[1];
            r.day = array[2];
            r.weekday = array[3];
            if(!rtc_setdate(&r)) return "Wrong date format\n";
        break;
        case 't': // set time
            if(3 != readNu8(buf+1, array, 3))
                return "Format: HH MM SS\n";
            r.hour = array[0];
            r.minute = array[1];
            r.second = array[2];
            if(!rtc_settime(&r)) return "Wrong time format\n";
        break;
        default:
            return "Sd -> set date; St -> set time\n";
    }
    return OK;
}

TRUE_INLINE const char *setcal(const char *buf){
    int32_t v;
    if(buf == getint(buf, &v)){
        USND("Calibration value: ");
        USND(i2str(rtc_getcalib()));
        putch('\n');
        return NULL;
    } 
    if(!rtc_setcalib(v)) return "Enter value: -511..+512\n";
    return OK;
}

const char *parse_cmd(char *buf){
    const char *x = omit_spaces(buf);
    // "long" commands
    switch(*x){
        case 'S':
            return setdatetime(x + 1);
        break;
        case 'C':
            return setcal(x + 1);
        break;
    }
    // "short" commands
    if(x[1]) return x; // echo wrong data
    switch(*x){
        case 't':
            USND("T=");
            USND(u2str(Tms));
            putch('\n');
        break;
        case 'T':
            print_curtime();
        break;
        default: // help
            USND(helpstring);
        break;
    }
    return NULL;
}
