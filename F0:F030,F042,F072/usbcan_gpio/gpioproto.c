/*
 * This file is part of the usbcangpio project.
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

#include <stm32f0.h>

#include "can.h"
#include "flash.h"
#include "gpioproto.h"

#define USBIF   IGPIO
#include "strfunc.h"

extern volatile uint32_t Tms;
static const char *const sOKn = "OK\n", *const sERRn = "ERR\n";

const char *helpstring =
    REPOURL
    "d  - dump flash\n"
    "ix - rename interface number x (0 - CAN, 1 - GPIO)\n"
    "Cx - starting CAN bus speed (kBaud)\n"
    "Dx - send text x to CAN USB interface\n"
    "E  - erase storage\n"
    "F  - reinit configurations from flash\n"
    "R  - soft reset\n"
    "S  - store new parameters into flash\n"
    "T  - print current Tms\n"
;

static void dumpflash(){
    SEND("userconf_sz="); SEND(u2str(the_conf.userconf_sz));
    SEND("\ncurrentconfidx="); SENDn(i2str(currentconfidx));
    for(int i = 0; i < InterfacesAmount; ++i){
        SEND("interface"); PUTCHAR('0' + i);
        PUTCHAR('=');
        int l = the_conf.iIlengths[i] / 2;
        char *ptr = (char*) the_conf.iInterface[i];
        for(int j = 0; j < l; ++j){
            PUTCHAR(*ptr);
            ptr += 2;
        }
        NL();
    }
    SEND("canspeed="); SENDn(u2str(the_conf.CANspeed));
}

// set new interface name
static const char* setiface(char *str){
    if(!str || !*str) goto err;
    uint32_t N;
    const char *nxt = getnum(str, &N);
    if(!nxt || nxt == str || N >= InterfacesAmount) goto err;
    //nxt = strchr(nxt, '=');
    //if(!nxt || !*(++nxt)) goto err;
    nxt = omit_spaces(nxt);
    if(!nxt || !*nxt) goto err;
    int l = strlen(nxt);
    if(l > MAX_IINTERFACE_SZ) goto err;
    the_conf.iIlengths[N] = (uint8_t) l * 2;
    char *ptr = (char*)the_conf.iInterface[N];
    for(int i = 0; i < l; ++i){
        char c = *nxt++;
        *ptr++ = (c > ' ') ? c : '_';
        *ptr++ = 0;
    }
    return sOKn;
err:
    return sERRn;
}

static const char* setCANspeed(char *buf){
    uint32_t N;
    if(buf == getnum(buf, &N)) return sERRn;
    if(N < CAN_MIN_SPEED || N > CAN_MAX_SPEED) return sERRn;
    the_conf.CANspeed = N;
    return sOKn;
}

static const char *cmd_parser(char *buf){
    if(!buf || !*buf) return NULL;
    if(strlen(buf) > 1){
        // "long" commands
        char c = *buf++;
        switch(c){
        case 'C':
            return setCANspeed(buf);
        case 'D':
            if(USB_sendstr(ICAN, buf)) return sOKn;
            else return sERRn;
        case 'i':
            return setiface(buf);
        default:
            return buf-1; // echo wrong data
        }
    }
    // "short" commands
    switch(*buf){
    case 'd':
        dumpflash();
        break;
    case 'E':
        if(erase_storage()) return sERRn;
        return sOKn;
    case 'F':
        flashstorage_init();
        return sOKn;
    case 'R':
        NVIC_SystemReset();
        return NULL;
    case 'S':
        if(store_userconf()) return sERRn;
        return sOKn;
    case 'T':
        SEND("T=");
        SENDn(u2str(Tms));
        break;
    default: // help
        SEND(helpstring);
        break;
    }
    return NULL;
}

void GPIO_process(){
    char inbuff[MAXSTRLEN];
    int l = RECV(inbuff, MAXSTRLEN);
    if(l < 0) SEND("ERROR: USB buffer overflow or string was too long\n");
    else if(l){
        const char *ans = cmd_parser(inbuff);
        if(ans) SEND(ans);
    }
}
