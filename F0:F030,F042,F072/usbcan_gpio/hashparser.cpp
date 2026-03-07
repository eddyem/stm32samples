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

#include <stdint.h>

extern "C"{
#include <stm32f0.h>
#include "can.h"
#include "flash.h"
#include "hashparser.h"
#include "gpioproto.h"
#define USBIF   IGPIO
#include "strfunc.h"
}

static const char *const sOKn = "OK\n", *const sERRn = "ERR\n";
extern uint32_t Tms;

// list of all commands and handlers
#define COMMAND_TABLE \
    COMMAND(canspeed,   "CAN bus speed setter/getter (kBaud, 10..1000)") \
    COMMAND(dumpflash,  "flash config dump") \
    COMMAND(time,       "Current time (ms)") \
    COMMAND(help,       "Show this help")

typedef struct {
    const char *name;
    const char *desc;
} CmdInfo;

// prototypes
#define COMMAND(name, desc)   static errcodes_t cmd_ ## name(const char*, char*);
    COMMAND_TABLE
#undef COMMAND

static const CmdInfo cmdInfo[] = { // command name, description - for `help`
#define COMMAND(name, desc)   { #name, desc },
    COMMAND_TABLE
#undef COMMAND
};

static errcodes_t cmd_canspeed(const char *cmd, char _U_ *args){
    SEND(cmd); PUTCHAR('='); SENDn(u2str(CAN_getspeed()));
    if(args && *args){SEND("You entered: "); SENDn(args);}
    return ERR_AMOUNT;
}

static errcodes_t cmd_dumpflash(const char _U_ *cmd, char _U_ *args){
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
    return ERR_AMOUNT;
}

static errcodes_t cmd_time(const char* cmd, char _U_ *args){
    SEND(cmd); PUTCHAR('='); SENDn(u2str(Tms));
    return ERR_AMOUNT;
}

static errcodes_t cmd_help(const char _U_ *cmd, char _U_ *args){
    SEND(REPOURL);
    for(size_t i = 0; i < sizeof(cmdInfo)/sizeof(cmdInfo[0]); i++){
        SEND(cmdInfo[i].name);
        SEND(" - ");
        SENDn(cmdInfo[i].desc);
    }
    return ERR_AMOUNT;
}

constexpr uint32_t hash(const char* str, uint32_t h = 0){
    return *str ? hash(str + 1, h + ((h << 7) ^ *str)) : h;
}

static const char* errtxt[ERR_AMOUNT] = {
    [ERR_OK] =   "OK",
    [ERR_BADPAR] =  "BADPAR",
    [ERR_BADVAL] = "BADVAL",
    [ERR_WRONGLEN] = "WRONGLEN",
    [ERR_CANTRUN] = "CANTRUN",
};


// TODO: add checking real command length!

void chk(char *str){
    if(!str || !*str) return;
    char command[CMD_MAXLEN+1];
    int i = 0;
    while(*str > '@' && i < CMD_MAXLEN){ command[i++] = *str++; }
    command[i] = 0;
    while(*str && *str <= ' ') ++str;
    char *restof = (char*) str;
    uint32_t h = hash(command);
    errcodes_t ecode = ERR_AMOUNT;
    switch(h){
#define COMMAND(name, desc) case hash(#name): ecode = cmd_ ## name(command, restof); break;
        COMMAND_TABLE
#undef COMMAND
        default: SEND("Unknown command, try 'help'\n"); break;
    }
    if(ecode < ERR_AMOUNT) SENDn(errtxt[ecode]);
    ;
}


/*
if(*args){
    const char *n = getnum(args, &N);
    if(n != args){ // get parameter
        if(N >= CANMESG_NOPAR){
            USB_sendstr(errtxt[ERR_BADPAR]); newline();
            return RET_GOOD;
        }
        par = (uint8_t) N;
    }
    n = strchr(n, '=');
    if(n){
        ++n;
        const char *nxt = getint(n, &val);
        if(nxt != n){ // set setter flag
            par |= SETTERFLAG;
        }
    }
}
*/
