/*
 * This file is part of the nitrogen project.
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

#include "hardware.h"
#include "proto.h"
#include "strfunc.h"
#include "version.inc"

#include <stm32f3.h>
#include <string.h>

// parno - number of parameter (or -1); cargs - string with arguments (after '=') (==NULL for getter), iarg - integer argument
static int goodstub(int _U_ parno, const char _U_ *carg, int32_t _U_ iarg){
    return RET_GOOD;
}

static int leds(int parno, const char *c, int32_t i){
    if(parno < 0){ // enable/disable all
        if(c){ LEDsON = i ? 1 : 0;}
        USB_sendstr("LED="); USB_sendstr(u2str(LEDsON));
    }else{
        if(parno >= LEDS_AMOUNT) return RET_WRONGARG;
        if(c) switch(i){
            case 2: LED_blink(parno); break;
            case 1: LED_on(parno); break;
            default: LED_off(parno);
        }
        USB_sendstr("LED"); USB_putbyte('0' + parno);
        USB_putbyte('='); USB_sendstr(u2str(LED_get(parno))); newline();
    }
    newline();
    return RET_GOOD;
}

typedef struct{
    int (*fn)(int, const char*, int32_t);
    const char *cmd;
    const char *help;
} commands;

commands cmdlist[] = {
    {goodstub, "stub", "simple stub"},
    {leds, "LED", "LEDx=y; where x=0..3 to work with single LED (then y=1-set, 0-reset, 2-toggle), absent to work with all (y=0 - disable, 1-enable)"},
    {NULL, NULL, NULL}
};

static void printhelp(){
    commands *c = cmdlist;
    USB_sendstr("https://github.com/eddyem/stm32samples/tree/master/F3:F303/NitrogenFlooding build#" BUILD_NUMBER " @ " BUILD_DATE "\n");
    while(c->fn){
        USB_sendstr(c->cmd);
        USB_sendstr(" - ");
        USB_sendstr(c->help);
        newline();
        ++c;
    }
}

static int parsecmd(const char *str){
    char cmd[CMD_MAXLEN + 1];
    if(!str || !*str) return RET_CMDNOTFOUND;
    int i = 0;
    while(*str > '@' && i < CMD_MAXLEN){ cmd[i++] = *str++; }
    cmd[i] = 0;
    int parno = -1;
    int32_t iarg = __INT32_MAX__;
    if(*str){
        uint32_t N;
        const char *nxt = getnum(str, &N);
        if(nxt != str) parno = (int) N;
        str = strchr(str, '=');
        if(str){
            str = omit_spaces(++str);
            getint(str, &iarg);
        }
    }
    commands *c = cmdlist;
    while(c->fn){
        if(strcmp(c->cmd, cmd) == 0) return c->fn(parno, str, iarg);
        ++c;
    }
    return RET_CMDNOTFOUND;
}


/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 */
const char *cmd_parser(const char *txt){
    int ret = parsecmd(txt);
    switch(ret){
        case RET_CMDNOTFOUND: printhelp(); return NULL; break;
        case RET_WRONGARG: return "Wrong command parameters\n"; break;
        case RET_GOOD: return NULL; break;
        default: return "FAIL\n"; break;
    }
}
