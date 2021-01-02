/*
 * This file is part of the ws2815 project.
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
#include "ws2815.h"

#define USND(str)  do{USB_send((uint8_t*)str, sizeof(str)-1);}while(0)

uint8_t S = 100, V = 50; // saturation and value
uint8_t pause = 0, rstcounter = 0;

const char *parse_cmd(const char *buf){
    if(buf[1] != '\n') return buf;
    switch(*buf){
        case 'c':
            rstcounter = 1;
        break;
        case 'p':
            pause = !pause;
        break;
        case 'R':
            USND("Soft reset\n");
            NVIC_SystemReset();
        break;
        case 's':
            if(S > 9) S -= 10;
            else S = 0;
        break;
        case 'S':
            if(S < 91) S += 10;
            else S = 100;
        break;
        case 'v':
            if(V > 9) V -= 10;
        break;
        case 'V':
            if(V < 91) V += 10;
        break;
        case 'W':
            USND("Wait for reboot\n");
            while(1){nop();};
        break;
        default: // help
            return
            "'c' - reset counter\n"
            "'p' - toggle pause\n"
            "'R' - software reset\n"
            "'s/S' - decrement/increment Saturation\n"
            "'v/V' - decrement/increment Value\n"
            "'W' - test watchdog\n"
            ;
        break;
    }
    return NULL;
}
