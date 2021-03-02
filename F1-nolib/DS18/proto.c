/*
 * This file is part of the DS18 project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "ds18.h"
#include "proto.h"
#include "usb.h"

extern volatile uint32_t Tms;

static const char *helpmesg =
        "'I' - get DS18 ID\n"
        "'M' - start measurement\n"
        "'P' - read scratchpad\n"
        "'S' - get DHT state\n"
        "'R' - software reset\n"
        "'T' - get time from start\n"
        "'W' - test watchdog\n"
;

const char *parse_cmd(const char *buf){
    if(buf[1] != '\n') return buf;
    switch(*buf){
        case 'I':
            if(DS18_readID()) return "Reading ID..\n";
            else return "Error\n";
        case 'M':
            if(DS18_start()) return "Started\n";
            else return "Wait a little\n";
        break;
/*        case 'p':
            DS18_poll();
            return "polling\n";
        break;*/
        case 'P':
            if(DS18_readScratchpad()) return "Reading scr..\n";
            else return "Error\n";
        break;
        case 'R':
            USB_send("Soft reset\n");
            NVIC_SystemReset();
        break;
        case 'S':
            switch(DS18_getstate()){
                case DS18_SLEEP:
                    return "Sleeping\n";
                case DS18_RESETTING:
                    return "Resetting\n";
                case DS18_DETECTING:
                    return "Detecting\n";
                case DS18_DETDONE:
                    return "Detection done\n";
                case DS18_READING:
                    return "Measuring\n";
                case DS18_GOTANSWER:
                    return "Results are ready\n";
                default:
                    return "Not found\n";
            }
        case 'T':
            USB_send("T=");
            USB_send(u2str(Tms));
            USB_send("ms\n");
        break;
        case 'W':
            USB_send("Wait for reboot\n");
            while(1){nop();};
        break;
        default: // help
            return helpmesg;
        break;
    }
    return NULL;
}

static char *_2str(uint32_t  val, uint8_t minus){
    static char strbuf[12];
    char *bufptr = &strbuf[11];
    *bufptr = 0;
    if(!val){
        *(--bufptr) = '0';
    }else{
        while(val){
            *(--bufptr) = val % 10 + '0';
            val /= 10;
        }
    }
    if(minus) *(--bufptr) = '-';
    return bufptr;
}

// return string with number `val`
char *u2str(uint32_t val){
    return _2str(val, 0);
}
char *i2str(int32_t i){
    uint8_t minus = 0;
    uint32_t val;
    if(i < 0){
        minus = 1;
        val = -i;
    }else val = i;
    return _2str(val, minus);
}

void printhex(uint8_t x){
    char b[3];
    uint8_t H(uint8_t c){
        if(c < 10)
            return (c + '0');
        else
            return (c + 'a' - 10);
    }
    b[0] = H(x >> 4);
    b[1] = H(x & 0x0f);
    b[2] = 0;
    USB_send(b);
}

void printT(int16_t T){ // print temperature value, this function should be called from ds18.c
    USB_send("Temperature="); USB_send(i2str(T));
    USB_send("\n");
}

void printsp(uint8_t *b, int n){
    USB_send("Regvalue:");
    for(int i = 0; i < n; ++i){
        USB_send(" 0x");
        printhex(b[i]);
    }
    USB_send("\n");
}
