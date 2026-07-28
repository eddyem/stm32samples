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
//#include "flash.h"
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
    COMMAND(help,       "show this help") \
    COMMAND(mcureset,   "reset MCU") \
    COMMAND(servo,      "get/set servoN value, mks (from " STR(SG90_MINPULSE) " to " STR(SG90_MAXPULSE) ")") \
    COMMAND(servopos,   "get/set servoN target position with given speed") \
    COMMAND(servospeed, "get/set servoN speed, mks per 20ms (1.." STR(SG90_MAXSPEED) ")") \
    COMMAND(time,       "show current time (ms)") \

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
    if(next != setter && parval){
        *parval = I32;
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
#if 0
static errcodes_t cmd_saveconf(const char*, char*){
    if(store_userconf()) return ERR_CANTRUN;
    return ERR_OK;
}

static errcodes_t cmd_eraseflash(const char*, char*){
    if(erase_storage(-1)) return ERR_CANTRUN;
    return ERR_OK;
}

static errcodes_t cmd_readconf(const char*, char* args){
    int32_t CHno = -1;
    splitargs(args, &CHno);
    if(CHno < 0 || CHno >= SENSORS_AMOUNT) return ERR_BADPAR;
    as3935_channel = static_cast<uint8_t>(CHno);
    uint8_t par;
    if(!as3935_get_gain(&par)) return ERR_CANTRUN;
    the_conf.spars[CHno].AFE_GB = par;
    if(!as3935_get_wdthres(&par)) return ERR_CANTRUN;
    the_conf.spars[CHno].WDTH = par;
    if(!as3935_get_nflev(&par)) return ERR_CANTRUN;
    the_conf.spars[CHno].NF_LEV = par;
    if(!as3935_get_srej(&par)) return ERR_CANTRUN;
    the_conf.spars[CHno].SREJ = par;
    if(!as3935_get_minnumlig(&par)) return ERR_CANTRUN;
    the_conf.spars[CHno].MIN_NUM_LIG = par;
    if(!as3935_get_maskdist(&par)) return ERR_CANTRUN;
    the_conf.spars[CHno].MASK_DIST = par;
    if(!as3935_get_lco_fdiv(&par)) return ERR_CANTRUN;
    the_conf.spars[CHno].LCO_FDIV = par;
    if(!as3935_get_tuncap(&par)) return ERR_CANTRUN;
    the_conf.spars[CHno].TUN_CAP = par;
    return ERR_OK;
}

static void showpar(const char *par, uint8_t n, uint8_t v){
    char c[2];
    c[0] = '0' + n; c[1] = 0;
    SEND(par); SEND(c); SEND(EQ);
    SEND(u2str(v));
}

static errcodes_t cmd_dumpconf(const char*, char*){
    SEND("userconf_sz="); SEND(u2str(the_conf.userconf_sz));
    SEND("\ncurr_idx="); SEND(u2str(currentconfidx));
    SEND("\ncapacity="); SEND(u2str(maxCnum-2));
    cmd_setiface("\nsetiface", NULL);
    cmd_restonstart("restonstart", NULL);
    for(int i = 0; i < SENSORS_AMOUNT; ++i){
        showpar("gain", i, the_conf.spars[i].AFE_GB);
        showpar("\nlco_fdiv", i, the_conf.spars[i].LCO_FDIV);
        showpar("\nmaskdist", i, the_conf.spars[i].MASK_DIST);
        showpar("\nminnumlig", i, the_conf.spars[i].MIN_NUM_LIG);
        showpar("\nnflev", i, the_conf.spars[i].NF_LEV);
        showpar("\nsrej", i, the_conf.spars[i].SREJ);
        showpar("\ntuncap", i, the_conf.spars[i].TUN_CAP);
        showpar("\nwdthres", i, the_conf.spars[i].WDTH);
        SEND("\n");
    }
    return ERR_AMOUNT;
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
