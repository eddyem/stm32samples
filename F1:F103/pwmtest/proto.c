/*
 * This file is part of the pwmtest project.
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

#include "proto.h"
#include "usb.h"
#include "hardware.h"

/**
 * @brief getnum - get uint32_t
 * @param buf (i) - string with text
 * @param N (o)   - number found
 * @return 1 if all OK
 */
static int getnum(const char *buf, uint32_t *N){
    char c;
    int found = 0;
    uint32_t val = 0;
    while((c = *buf)){
        if(c == '\t' || c == ' ') ++buf;
        else break;
    }
    while((c = *buf++)){
        if(c < '0' || c > '9') break;
        val = val * 10 + (uint32_t)(c - '0');
        found = 1;
    }
    if(found){
        *N = val;
        return 1;
    }
    return 0;
}


const char *parse_cmd(const char *buf){
    uint32_t U = 0;
#define GETN()  getnum(buf+1, &U)
    if(buf[1] != '\n'){
        switch(*buf){ // multisymbol commands
        case 'D': // change duty
            if(GETN()){
                if(changePWMduty(U)) return "OK\n";
                else return "Wrong\n";
            }else return "Wrong number\n";
        break;
        case 'F': // change frequency
            if(GETN()){
                if(changePWMfreq(U)) return "OK\n";
                else return "Wrong\n";
            }else return "Wrong number\n";
        break;
        default:
            return buf;
        }
    }
    switch(*buf){
        case '0':
            PWM_OFF();
            return "PWM is off\n";
        break;
        case '1':
            PWM_ON();
            return "PWM is on\n";
        break;
        case 'R':
            USB_send("Soft reset\n");
            NVIC_SystemReset();
        break;
        case 'W':
            USB_send("Wait for reboot\n");
            while(1){nop();};
        break;
        default: // help
            return
            "'0' - turn OFF PWM\n"
            "'1' - turn ON PWM\n"
            "'D n' - change duty (promille)\n"
            "'F n' - change freq (Hz)\n"
            "'R' - software reset\n"
            "'W' - test watchdog\n"
            ;
        break;
    }
    return NULL;
}

static char strbuf[11];
// return string buffer (strbuf) with val
char *u2str(uint32_t val){
    char *bufptr = &strbuf[10];
    *bufptr = 0;
    if(!val){
        *(--bufptr) = '0';
    }else{
        while(val){
            *(--bufptr) = val % 10 + '0';
            val /= 10;
        }
    }
    return bufptr;
}
