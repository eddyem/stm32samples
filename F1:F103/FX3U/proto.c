/*
 * This file is part of the fx3u project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "adc.h"
#include "can.h"
#include "hardware.h"
#include "proto.h"
#include "strfunc.h"
#include "usart.h"
#include "version.inc"

flags_t flags = {
    .can_monitor = 0
};

/*
static void printans(int res){
    if(res) usart_send("OK");
    else usart_send("FAIL");
}

// setters of uint/int
static void usetter(int(*fn)(uint32_t), char* str){
    uint32_t d = 0;
    if(str == getnum(str, &d)) printans(FALSE);
    else printans(fn(d));
}
static void isetter(int(*fn)(int32_t), char* str){
    int32_t d = 0;
    if(str == getint(str, &d)) printans(FALSE);
    else printans(fn(d));
}
*/

// parno - number of parameter (or -1); cargs - string with arguments (after '=') (==NULL for getter), iarg - integer argument
static int goodstub(const char *cmd, int parno, const char *carg, int32_t iarg){
    usart_send("cmd="); usart_send(cmd);
    usart_send(", parno="); usart_send(i2str(parno));
    usart_send(", args="); usart_send(carg);
    usart_send(", intarg="); usart_send(i2str(iarg)); newline();
    return RET_GOOD;
}

typedef struct{
    int (*fn)(const char*, int, const char*, int32_t);
    const char *cmd;
    const char *help;
} commands;

static commands cmdlist[] = {
    {goodstub, "stub", "simple stub"},
    {NULL, "Different commands", NULL},
//    {adcval, "ADC", "get ADCx value (without x - for all)"},
//    {adcvoltage, "ADCv", "get ADCx voltage (without x - for all)"},
//    {mcut, "mcut", "get MCU temperature"},
    {NULL, NULL, NULL}
};

static void printhelp(){
    commands *c = cmdlist;
    usart_send("https://github.com/eddyem/stm32samples/tree/master/F1:F103/FX3U#" BUILD_NUMBER " @ " BUILD_DATE "\n");
    while(c->cmd){
        if(!c->fn){ // header
            usart_send("\n    ");
            usart_send(c->cmd);
            usart_putchar(':');
        }else{
            usart_send(c->cmd);
            usart_send(" - ");
            usart_send(c->help);
        }
        newline();
        ++c;
    }
}

/**
 * @brief parsecmd - parse text commands over RS-232
 * @param str - input string
 * @return answer code
 */
static int parsecmd(const char *str){
    char cmd[CMD_MAXLEN + 1];
    //USB_sendstr("cmd="); USB_sendstr(str); USB_sendstr("__\n");
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
    }else str = NULL;
    commands *c = cmdlist;
    while(c->cmd){
        if(strcmp(c->cmd, cmd) == 0){
            if(!c->fn) return RET_CMDNOTFOUND;
            return c->fn(cmd, parno, str, iarg);
        }
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
    case RET_WRONGPARNO: return "Wrong parameter number\n"; break;
    case RET_CMDNOTFOUND: printhelp(); return NULL; break;
    case RET_WRONGARG: return "Wrong command parameters\n"; break;
    case RET_GOOD: return NULL; break;
    default: return "FAIL\n"; break;
    }
}
