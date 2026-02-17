/*
 * This file is part of the multiiface project.
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

#include <stm32f3.h>
#include <string.h>

#include "flash.h"
#include "strfunc.h"
#include "usart.h"
#include "usb_dev.h"
#include "version.inc"

// all functions of this file could be run only in configuration mode

static const char *const sOKn = "OK\n", *const sERRn = "ERR\n";

//#define LOCBUFFSZ       (32)
// local buffer for I2C data to send
//static uint8_t locBuffer[LOCBUFFSZ];
extern volatile uint32_t Tms;

const char *helpstring =
        "https://github.com/eddyem/stm32samples/tree/master/F3:F303/InterfaceBoard build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
        "1..5x - send data over IF1..5\n"
        "d - dump flash\n"
        "ix - rename interface number x (0..6)\n"
        "Ex - erase full storage (witout x) or only page x\n"
        "F - reinit configurations from flash\n"
        "R - soft reset\n"
        "S - store new parameters into flash\n"
        "T - print current Tms\n"
;

static void dumpflash(){
    CFGWR("userconf_sz="); CFGWR(u2str(the_conf.userconf_sz));
    CFGWR("\ncurrentconfidx="); CFGWRn(i2str(currentconfidx));
    for(int i = 0; i < InterfacesAmount; ++i){
        CFGWR("interface"); USB_putbyte(ICFG, '0' + i);
        USB_putbyte(ICFG, '=');
        int l = the_conf.iIlengths[i] / 2;
        char *ptr = (char*) the_conf.iInterface[i];
        for(int j = 0; j < l; ++j){
            USB_putbyte(ICFG, *ptr);
            ptr += 2;
        }
        CFGn();
    }
}

static void setiface(const char *str){
    if(!str || !*str) goto err;
    uint32_t N;
    const char *nxt = getnum(str, &N);
    if(!nxt || nxt == str || N >= InterfacesAmount) goto err;
    nxt = strchr(nxt, '=');
    if(!nxt || !*(++nxt)) goto err;
    nxt = omit_spaces(nxt);
    if(!nxt || !*nxt) goto err;
    int l = strlen(nxt);
    if(l > MAX_IINTERFACE_SZ) goto err;
    the_conf.iIlengths[N] = (uint8_t) l * 2;
    char *ptr = (char*)the_conf.iInterface[N];
    for(int i = 0; i < l; ++i){
        *ptr++ = *nxt++;
        *ptr++ = 0;
    }
    CFGWR(sOKn);
    return;
err:
    CFGWR(sERRn);
}

static const char* erpg(const char *str){
    uint32_t N;
    if(str == getnum(str, &N)) return sERRn;
    if(erase_storage(N)) return sERRn;
    return sOKn;
}

static void sendoverU(uint8_t ifno, char *str){
    int len = strlen(str);
    CFGWR("try to send "); CFGWRn(str);
    len = usart_send(ifno, (const uint8_t*)str, len);
    CFGWR("sent "); CFGWR(i2str(len)); CFGWR("bytes\n");
}

const char *parse_cmd(char *buf){
    if(!buf || !*buf) return NULL;
    if(strlen(buf) > 1){
        // "long" commands
        char c = *buf++;
        if(c > '0' && c < '6'){
            sendoverU(c - '1', buf);
            return NULL;
        }
        switch(c){
            case 'E':
                return erpg(buf);
            case 'i':
                setiface(buf);
                return NULL;
            break;
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
            if(erase_storage(-1)) return sERRn;
            return sOKn;
        break;
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
            CFGWR("T=");
            CFGWRn(u2str(Tms));
            break;
        default: // help
            CFGWR(helpstring);
        break;
    }
    return NULL;
}
