/*
 * This file is part of the DHT22 project.
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

#include "dht.h"
#include "proto.h"
#include "usb.h"

extern volatile uint32_t Tms;

static const char *helpmesg =
        "'M' - start measurement\n"
        "'S' - get DHT state\n"
        "'R' - software reset\n"
        "'W' - test watchdog\n"
;

const char *parse_cmd(const char *buf){
    if(buf[1] != '\n') return buf;
    switch(*buf){
        case 'M':
            if(DHT_start(Tms)) return "Started\n";
            else return "Wait a little\n";
        break;
        case 'R':
            USB_send("Soft reset\n");
            NVIC_SystemReset();
        break;
        case 'S':
            switch(DHT_getstate()){
                case DHT_SLEEP:
                    return "Sleeping\n";
                case DHT_RESETTING:
                    return "Resetting\n";
                case DHT_MEASURING:
                    return "Measuring\n";
                case DHT_GOTRESULT:
                    return "Results are ready\n";
                default:
                    return "Not found\n";
            }
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
