/*
 * This file is part of the encoders project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <string.h>

#include "proto.h"
#include "strfunc.h"
#include "usb_dev.h"
#include "version.inc"

// commands indexes
typedef enum{
    C_dummy,
    C_help,
    C_sendX,
    C_sendY,
    C_AMOUNT
} cmd_e;

// error codes
typedef enum {
    ERR_OK,         // no errors
    ERR_FAIL,       // failed to run command
    ERR_BADCMD,     // bad command
    ERR_BADPAR,     // bad parameter (absent or wrong value)
    ERR_SILENCE,    // getter or other - don't need to send "OK"
    ERR_AMOUNT
} errcode_e;

static const char* const errors[ERR_AMOUNT] = {
    [ERR_OK] = "OK",
    [ERR_FAIL] = "FAIL",
    [ERR_BADCMD] = "BADCMD",
    [ERR_BADPAR] = "BADPAR",
    [ERR_SILENCE] = "",
};

// command handler (idx - index of command in list, par - all after equal sign in "cmd = par")
typedef errcode_e (*handler_t)(cmd_e idx, char *par);

typedef struct{
    const char* cmd;    // command
    handler_t handler;  // handler function
} funcdescr_t;

static const funcdescr_t commands[C_AMOUNT];

// getter for integer parameter
#define IGETTER(idx, ival)  do{CMDWR(commands[idx].cmd); USB_putbyte(I_CMD, '='); \
                            CMDWR(i2str(ival)); CMDn(); return ERR_SILENCE; }while(0)

static errcode_e help(cmd_e idx, char* par);

static errcode_e dummy(cmd_e idx, char *par){
    static int32_t val = -111;
    if(par && *par){
        int32_t i;
        char *nxt = getint(par, &i);
        DBGs(par);
        DBGs(nxt);
        if(par == nxt) return ERR_BADPAR;
        val = i;
    }
    IGETTER(idx, val);
}

static errcode_e sendenc(cmd_e idx, char *par){
    if(!par || !*par) return ERR_BADPAR;
    switch(idx){
        case C_sendX:
            if(!CDCready[I_X]) return ERR_FAIL;
            if(!USB_sendstr(I_X, par)) return ERR_FAIL;
            newline(I_X);
        break;
        case C_sendY:
            if(!CDCready[I_Y]) return ERR_FAIL;
            if(!USB_sendstr(I_Y, par)) return ERR_FAIL;
            newline(I_Y);
        break;
        default:
            return ERR_BADCMD;
    }
    return ERR_OK;
}

// text commands
static const funcdescr_t commands[C_AMOUNT] = {
    [C_dummy] = {"dummy", dummy},
    [C_help] = {"help", help},
    [C_sendX] = {"sendx", sendenc},
    [C_sendY] = {"sendy", sendenc},
    };

typedef struct{
    int idx;            // command index (if < 0 - just display `help` as grouping header)
    const char *help;   // help message
} help_t;

// SHOUL be sorted and grouped
static const help_t helpmessages[] = {
    {-1, "Different commands"},
    {C_dummy, "dummy integer setter/getter"},
    {C_help, "show this help"},
    {-1, "Debug commands"},
    {C_sendX, "send text string to X encoder"},
    {C_sendY, "send text string to Y encoder"},
    {-1, NULL},
};

static errcode_e help(_U_ cmd_e idx, _U_ char*  par){
    CMDWRn("https://github.com/eddyem/stm32samples/tree/master/F1:F103/ build #" BUILD_NUMBER " @ " BUILD_DATE);
    CMDWRn("commands format: command[=setter]\\n");
    const help_t *c = helpmessages;
    while(c->help){
        if(c->idx < 0){ // header
            CMDWR("\n    ");
            CMDWR(c->help);
            USB_putbyte(I_CMD, ':');
        }else{
            CMDWR(commands[c->idx].cmd);
            CMDWR(" - ");
            CMDWR(c->help);
        }
        CMDn();
        ++c;
    }
    return ERR_OK;
}

// *cmd is "command" for commands/getters or "parameter=value" for setters
void parse_cmd(char *cmd){
    errcode_e ecode = ERR_BADCMD;
    // command and its parameter
    char *cmdstart = omit_spaces(cmd), *parstart = NULL;
    if(!cmdstart) goto retn;
    char *ptr = cmdstart;
    while(*ptr > '@') ++ptr;
    if(*ptr && *ptr <= ' '){ // there's spaces after command
        *ptr++ = 0;
        ptr = omit_spaces(ptr);
    }
    if(*ptr){ // check rest of string
        if(*ptr != '=') goto retn; // strange symbols after command name
        *ptr++ = 0;
        parstart = omit_spaces(ptr); // get parameter
        if(*parstart <= ' ') goto retn; // no parameter after equal sign
    }
    int idx = 0;
    for(; idx < C_AMOUNT; ++idx) // search in command lists
        if(0 == strcmp(commands[idx].cmd, cmdstart)) break;
    if(idx >= C_AMOUNT) goto retn; // not found
    ecode = commands[idx].handler(idx, parstart);
retn:
    if(ecode != ERR_SILENCE) CMDWRn(errors[ecode]);
}
