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
        "t - print current Tms\n"
        "T - print current Time\n"
;

TRUE_INLINE void putch(char x){
    usart3_send(&x, 1);
}

static const char *weekdays[] = {"Mon", "Tue", "Wed", "Thur", "Fri", "Sat", "Sun"};
static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sept", "Oct", "Nov", "Dec"};

static void prezero(uint8_t x){
    if(x < 10){ putch('0'); putch('0' + x);}
    else USND(u2str(x));
}

void print_curtime(){
    rtc_t t;
    get_curtime(&t);
    USND(weekdays[t.day - 1]);
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

char *parse_cmd(char *buf){
    // "long" commands
/*    switch(*buf){
    }*/
    // "short" commands
    if(buf[1]) return buf; // echo wrong data
    switch(*buf){
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
