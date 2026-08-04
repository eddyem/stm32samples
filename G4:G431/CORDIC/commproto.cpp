/*
 * This file is part of the cordic project.
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

#include <cstring>

extern "C"{
#include <stm32g4.h>

#include "commproto.h"
#include "hardware.h"
#include "strfunc.h"
#include "test.h"
}

// sending function
static int (*SEND)(const char *str) = nullptr;

extern volatile uint32_t Tms;

//static uint8_t curbuf[MAXSTRLEN];

// COMMAND(USART,      "Read USART data or send (USART=hex)")

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// list of all commands and handlers
#define COMMAND_TABLE \
    COMMAND(help,       "show this help") \
    COMMAND(testm,      "test math function: sin, cos, atan, sqrt, log") \
    COMMAND(testc,      "test CORDIC function: sin, cos, atan, sqrt, log")


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

static const char* errtxt[ERR_AMOUNT] = {
    [ERR_OK]        = "OK\n",
    [ERR_BADCMD]    = "BADCMD\n",
    [ERR_BADPAR]    = "BADPAR\n",
    [ERR_BADVAL]    = "BADVAL\n",
    [ERR_WRONGLEN]  = "WRONGLEN\n",
    [ERR_CANTRUN]   = "CANTRUN\n",
    [ERR_BUSY]      = "BUSY\n",
    [ERR_OVERFLOW]  = "OVERFLOW\n",
};

const char *EQ = " = "; // equal sign for getters

// send `command = `
#define CMDEQ()   do{SEND(cmd); SEND(EQ);}while(0)
// send `commandXXX = `
#define CMDEQP(x)   do{SEND(cmd); SEND(u2str((uint32_t)x)); SEND(EQ);}while(0)
// the same as last but with command as option and uint value
#define SHOWPARU(cmd, x, val)  do{SEND(cmd); SEND(u2str((uint32_t)x)); SEND(EQ); SEND(u2str(val));}while(0)

/**
 * @brief splitargs - get command parameter and setter from `args`
 * @param args (i) - rest of string after command (like `1 = PU OD OUT`)
 * @param parno (o) - parameter number or -1 if none
 * @return setter (part after `=` without leading spaces) or NULL if none
 */
static char *splitargs(char *args, int32_t *parno){
    if(!args) return NULL;
    uint32_t U32;
    char *next = getnum(args, &U32);
    int p = -1;
    if(next != args && U32 <= MAXPARNO) p = U32;
    if(parno) *parno = p;
    next = strchr(next, '=');
    if(next){
        if(*(++next)) next = omit_spaces(next);
        if(*next == 0) next = NULL;
    }
    return next;
}

#if 0
/**
 * @brief argsvals - split `args` into `parno` and setter's value
 * @param args - rest of string after command
 * @param parno (o) - parameter number or -1 if none
 * @param parval - integer setter's value
 * @return false if no setter or it's not a number, true - got setter's num
 */
static bool argsvals(char *args, int32_t *parno, int32_t *parval){
    char *setter = splitargs(args, parno);
    if(!setter) return false;
    int32_t I32;
    char *next = getint(setter, &I32);
    if(next != setter){
        if(parval) *parval = I32;
        return true;
    }
    return false;
}
#endif

static errcodes_t cmd_help(const char*, char*){
    SEND(REPOURL);
    for(size_t i = 0; i < sizeof(cmdInfo)/sizeof(cmdInfo[0]); i++){
        SEND(cmdInfo[i].name);
        SEND(" - ");
        SEND(cmdInfo[i].desc); SEND("\n");
    }
    return ERR_AMOUNT;
}

static const char* parse_func_name(char *args, int32_t *parno){
    char *setter = splitargs(args, parno);
    if(!setter) return nullptr;
    // remove trailing spaces
    char *p = setter;
    while(*p && *p > ' ') ++p;
    *p = 0;
    return setter;
}

// test math function
static errcodes_t cmd_testm(const char*, char *args){
    int32_t parno;
    const char *fname = parse_func_name(args, &parno);
    if(!fname) return ERR_BADPAR;
    uint32_t elapsed = 0;
    bool ok = true;
    if(strcmp(fname, "sin") == 0){
        elapsed = test_math_sin();
    }else if(strcmp(fname, "cos") == 0){
        elapsed = test_math_cos();
    }else if(strcmp(fname, "atan") == 0){
        elapsed = test_math_atan();
    }else if(strcmp(fname, "sqrt") == 0){
        elapsed = test_math_sqrt();
    }else if(strcmp(fname, "log") == 0){
        elapsed = test_math_log();
    }else{
        ok = false;
    }
    if(!ok) return ERR_BADVAL;
    SEND("TIMEus=");
    SEND(u2str(elapsed));
    SEND("\n");
    return ERR_AMOUNT;
}


// test CORDIC function
static errcodes_t cmd_testc(const char*, char *args){
    int32_t parno;
    const char *fname = parse_func_name(args, &parno);
    if(!fname) return ERR_BADPAR;
    uint32_t elapsed = 0;
    bool ok = true;
    if(strcmp(fname, "sin") == 0){
        elapsed = test_cordic_sin();
    }else if(strcmp(fname, "cos") == 0){
        elapsed = test_cordic_cos();
    }else if(strcmp(fname, "atan") == 0){
        elapsed = test_cordic_atan();
    }else if(strcmp(fname, "sqrt") == 0){
        elapsed = test_cordic_sqrt();
    }else if(strcmp(fname, "log") == 0){
        elapsed = test_cordic_log();
    }else{
        ok = false;
    }
    if(!ok) return ERR_BADVAL;
    SEND("TIMEus=");
    SEND(u2str(elapsed));
    SEND("\n");
    return ERR_AMOUNT;
}

constexpr uint32_t hash(const char* str, uint32_t h = 0){
    return *str ? hash(str + 1, h + ((h << 7) ^ *str)) : h;
}

const char *parse_cmd(int (*sendfun)(const char *), char *str){
    SEND = sendfun;
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
    if(ecode < ERR_AMOUNT) return errtxt[ecode];
    return NULL;
}
