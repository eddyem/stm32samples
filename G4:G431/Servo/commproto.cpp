/*
 * This file is part of the as3935 project.
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
#include "flash.h"
#include "hardware.h"
#include "servo.h"
#include "strfunc.h"
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
    COMMAND(dumpconf,   "dump current config") \
    COMMAND(eraseflash, "erase full flash storage") \
    COMMAND(help,       "show this help") \
    COMMAND(maxpos,     "servoN maximal allowed position (conf)") \
    COMMAND(maxspeed,   "servoN maximal allowed speed (from 1 to " STR(SERVO_MAXSPEED) ", conf)") \
    COMMAND(mcureset,   "reset MCU") \
    COMMAND(minpos,     "servoN minimal allowed position (conf)") \
    COMMAND(readconf,   "re-read config from flash") \
    COMMAND(saveconf,   "save config to flash") \
    COMMAND(servo,      "servoN value, mks (in allowed range)") \
    COMMAND(servopos,   "servoN target position with given speed") \
    COMMAND(servospeed, "servoN speed, mks per 20ms (1..maxspeed)") \
    COMMAND(startpos,   "servoN started position (from " STR(SERVO_MINPULSE) " to " STR(SERVO_MAXPULSE) ", conf)") \
    COMMAND(time,       "show current time (ms)") \
    COMMAND(usart_speed,"speed of USART (set after reset, conf)") \


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

static errcodes_t cmd_servo(const char *cmd, char *args){
    int32_t val, parno;
    if(argsvals(args, &parno, &val)){ // setter
        if(parno < 0 || parno >= SERVO_AMOUNT) return ERR_BADPAR;
        if(!set_servo((uint8_t)parno, (uint16_t)val)) return ERR_BADVAL;
    }
    uint16_t curval;
    if(!get_servo((uint8_t)parno, &curval)) return ERR_BADPAR;
    CMDEQP(parno);
    SEND(u2str(curval));
    SEND("\n");
    return ERR_AMOUNT;
}

static errcodes_t cmd_servopos(const char *cmd, char *args){
    int32_t val, parno;
    if(argsvals(args, &parno, &val)){ // setter
        if(parno < 0 || parno >= SERVO_AMOUNT) return ERR_BADPAR;
        if(!servo_set_tagpos((uint8_t)parno, (uint16_t)val)) return ERR_BADVAL;
    }
    uint16_t curval;
    if(!servo_get_tagpos((uint8_t)parno, &curval)) return ERR_BADPAR;
    CMDEQP(parno);
    SEND(u2str(curval));
    SEND("\n");
    return ERR_AMOUNT;
}

static errcodes_t cmd_servospeed(const char *cmd, char *args){
    int32_t val, parno;
    if(argsvals(args, &parno, &val)){ // setter
        if(parno < 0 || parno >= SERVO_AMOUNT) return ERR_BADPAR;
        if(!servo_set_speed((uint8_t)parno, (uint16_t)val)) return ERR_BADVAL;
    }
    uint16_t curval;
    if(!servo_get_speed((uint8_t)parno, &curval)) return ERR_BADPAR;
    CMDEQP(parno);
    SEND(u2str(curval));
    SEND("\n");
    return ERR_AMOUNT;
}

static errcodes_t cmd_time(const char *cmd, char*){
    CMDEQ();
    SEND(u2str(Tms)); SEND("\n");
    return ERR_AMOUNT;
}

static errcodes_t cmd_mcureset(const char*, char*){
    NVIC_SystemReset();
    return ERR_CANTRUN; // never reached
}

static errcodes_t cmd_saveconf(const char*, char*){
    if(store_userconf()) return ERR_CANTRUN;
    return ERR_OK;
}

static errcodes_t cmd_eraseflash(const char*, char*){
    if(erase_storage()) return ERR_CANTRUN;
    return ERR_OK;
}

static errcodes_t cmd_readconf(const char*, char*){
    flashstorage_init();
    return ERR_OK;
}

static errcodes_t cmd_dumpconf(const char*, char*){
    SEND("userconf_sz="); SEND(u2str(the_conf.userconf_sz));
    SEND("\ncurr_idx="); SEND(i2str(currentconfidx));
    SEND("\ncapacity="); SEND(u2str(maxCnum-2));
    SEND("\nusart_speed="); SEND(u2str(the_conf.usart_speed));
    for(int i = 0; i < SERVO_AMOUNT ; ++i){
        SHOWPARU("\nstartpos", i, the_conf.startpulse[i]);
        SHOWPARU("\nminpos", i, the_conf.minpulse[i]);
        SHOWPARU("\nmaxpos", i, the_conf.maxpulse[i]);
        SHOWPARU("\nmaxspeed", i, the_conf.maxspeed[i]);
    }
    SEND("\n");
    return ERR_AMOUNT;
}

static errcodes_t cmd_usart_speed(const char* cmd, char* args){
    int32_t val;
    if(argsvals(args, NULL, &val)){ // setter
        if(val < MIN_USART_SPEED || val > MAX_USART_SPEED) return ERR_BADVAL;
        the_conf.usart_speed = (uint32_t) val;
    }
    CMDEQ();
    SEND(u2str(the_conf.usart_speed));
    SEND("\n");
    return ERR_AMOUNT;
}

static errcodes_t servo_valP(const char* cmd, char* args, uint16_t *confval){
    int32_t val, parno;
    if(argsvals(args, &parno, &val)){ // setter
        if(parno < 0 || parno >= SERVO_AMOUNT) return ERR_BADPAR;
        if(val < SERVO_MINPULSE || val > SERVO_MAXPULSE) return ERR_BADVAL;
        confval[parno] = (uint16_t) val;
    }
    SHOWPARU(cmd, parno, confval[parno]);
    SEND("\n");
    return ERR_AMOUNT;
}

static errcodes_t cmd_maxpos(const char* cmd, char* args){
    return servo_valP(cmd, args, the_conf.maxpulse);
}

static errcodes_t cmd_minpos(const char* cmd, char* args){
    return servo_valP(cmd, args, the_conf.minpulse);
}

static errcodes_t cmd_startpos(const char* cmd, char* args){
    return servo_valP(cmd, args, the_conf.startpulse);
}

static errcodes_t cmd_maxspeed(const char* cmd, char* args){
    int32_t val, parno;
    if(argsvals(args, &parno, &val)){ // setter
        if(parno < 0 || parno >= SERVO_AMOUNT) return ERR_BADPAR;
        if(val < 1 || val > SERVO_MAXSPEED) return ERR_BADVAL;
        the_conf.maxspeed[parno] = (uint16_t) val;
    }
    SHOWPARU(cmd, parno, the_conf.maxspeed[parno]);
    SEND("\n");
    return ERR_AMOUNT;
}

static errcodes_t cmd_help(const char*, char*){
    SEND(REPOURL);
    for(size_t i = 0; i < sizeof(cmdInfo)/sizeof(cmdInfo[0]); i++){
        SEND(cmdInfo[i].name);
        SEND(" - ");
        SEND(cmdInfo[i].desc); SEND("\n");
    }
    return ERR_AMOUNT;
}

#if 0
/**
 * @brief parse_hex_data - data parsing in case of `hex + text` input format
 * @param input - input string
 * @param output - output data
 * @param max_len - length of `output`
 * @return amount of parsed bytes or -1 in case of overflow or error
 */
static int parse_hex_data(char *input, uint8_t *output, int max_len){
    if(!input || !*input || !output || max_len < 1) return 0;
    char *p = input;
    int out_idx = 0;
    while(*p && out_idx < max_len){
        while(*p == ' ' || *p == ',') ++p; // omit spaces and commas as delimeters
        if(*p == '\0') break; // EOL
        if(*p == '"'){ // TEXT (start/end)
            ++p;
            while(*p && *p != '"'){
                if(out_idx >= max_len) return -1;
                output[out_idx++] = *p++;
            }
            if(*p == '"'){
                ++p; // go to next symbol after closing quotation mark
            }else return -1; // no closing
        }else{ // HEX number
            char *start = p;
            while(*p && *p != ' ' && *p != ',' && *p != '"') ++p;
            char saved = *p;
            *p = '\0'; // temporarily for `gethex`
            uint32_t val;
            const char *end = gethex(start, &val);
            if(end != p || val > 0xFF){ // not a hex number or have more than 2 symbols
                *p = saved;
                return -1;
            }
            *p = saved;
            output[out_idx++] = (uint8_t)val;
        }
    }
    return out_idx;
}
#endif

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
