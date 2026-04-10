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
#include <stm32f1.h>

#include "adc.h"
#include "as3935.h"
#include "commproto.h"
#include "hardware.h"
#include "spi.h"
#include "strfunc.h"
}

// sending function
static int (*SEND)(const char *str) = nullptr;

extern volatile uint32_t Tms;

static uint8_t curbuf[MAXSTRLEN];

// COMMAND(dumpconf,   "dump global configuration")
// COMMAND(eraseflash, "erase full flash storage")
// COMMAND(readconf,   "re-read config from flash")
// COMMAND(saveconf,   "save current user configuration into flash")
// COMMAND(USART,      "Read USART data or send (USART=hex)")

// list of all commands and handlers
#define COMMAND_TABLE \
    COMMAND(clearstat,  "clear amount of lightnings for last 15 min") \
    COMMAND(displco,    "display on LCO: 0- nothing, 1- TRCO, 2- SRCO, 3- LCO") \
    COMMAND(distance,   "distance to lightning, km") \
    COMMAND(energy,     "energy of last lightning") \
    COMMAND(gain,       "change sensor's gain (0..1f)") \
    COMMAND(help,       "show this help") \
    COMMAND(intcode,    "last interrupt code") \
    COMMAND(lco_fdiv,   "set Fdiv of antenna LCO") \
    COMMAND(maskdist,   "mask (1) or unmask (0) disturber") \
    COMMAND(mcutemp,    "get MCU temperature (degC*10)") \
    COMMAND(mcureset,   "reset MCU") \
    COMMAND(minnumlig,  "ninimal lightnings number (0..2)") \
    COMMAND(nflev,      "noice floor level (0..7)") \
    COMMAND(resetdef,   "reset sensor to defaults") \
    COMMAND(SPI,        "transfer SPI data: SPIx=data (hex)") \
    COMMAND(srej,       "spike rejection (0..15)") \
    COMMAND(time,       "show current time (ms)") \
    COMMAND(tuncap,     "set 'tune capasitors' to given value") \
    COMMAND(vdd,        "get approx Vdd value (V*100)") \
    COMMAND(wakeup,     "wake-up given sensor and make its calibration") \
    COMMAND(wdthres,    "watchdog threshold (0..15)" )

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

static const char *EQ = " = "; // equal sign for getters

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

static errcodes_t cmd_time(const char *cmd, char _U_ *args){
    CMDEQ();
    SEND(u2str(Tms)); SEND("\n");
    return ERR_AMOUNT;
}

static errcodes_t cmd_mcureset(const char _U_ *cmd, char _U_ *args){
    NVIC_SystemReset();
    return ERR_CANTRUN; // never reached
}
#if 0
static errcodes_t cmd_readconf(const char _U_ *cmd, char _U_ *args){
    flashstorage_init();
    return ERR_OK;
}

static errcodes_t cmd_saveconf(const char _U_ *cmd, char _U_ *args){
    if(store_userconf()) return ERR_CANTRUN;
    return ERR_OK;
}

static errcodes_t cmd_eraseflash(const char _U_ *cmd, char _U_ *args){
    if(erase_storage()) return ERR_CANTRUN;
    return ERR_OK;
}
#endif


static errcodes_t cmd_mcutemp(const char *cmd, char _U_ *args){
    CMDEQ();
    SEND(i2str(getMCUtemp())); SEND("\n");
    return ERR_AMOUNT;
}

static errcodes_t cmd_vdd(const char *cmd, char _U_ *args){
    CMDEQ();
    SEND(u2str(getVdd())); SEND("\n");
    return ERR_AMOUNT;
}

static errcodes_t cmd_help(const char _U_ *cmd, char _U_ *args){
    SEND(REPOURL);
    for(size_t i = 0; i < sizeof(cmdInfo)/sizeof(cmdInfo[0]); i++){
        SEND(cmdInfo[i].name);
        SEND(" - ");
        SEND(cmdInfo[i].desc); SEND("\n");
    }
    return ERR_AMOUNT;
}

static errcodes_t senscmd8(int32_t CHno, int (*cmd)(uint8_t), int32_t arg){
    if(CHno < 0 || CHno >= SENSORS_AMOUNT) return ERR_BADPAR;
    uint8_t par = static_cast<uint8_t>(arg);
    CS(CHno);
    int ans = cmd(par);
    CS_OFF();
    if(ans) return ERR_OK;
    return ERR_CANTRUN;
}

static errcodes_t getta(const char *cmd, int32_t CHno, int (*fn)(uint8_t*)){
    if(CHno < 0 || CHno >= SENSORS_AMOUNT) return ERR_BADPAR;
    uint8_t par;
    CS(CHno);
    int ans = fn(&par);
    CS_OFF();
    if(!ans) return ERR_CANTRUN;
    CMDEQP(CHno); SEND(u2str(par)); SEND("\n");
    return ERR_AMOUNT;
}

static errcodes_t senscmd(int32_t CHno, int (*cmd)()){
    if(CHno < 0 || CHno >= SENSORS_AMOUNT) return ERR_BADPAR;
    CS(CHno);
    int ans = cmd();
    CS_OFF();
    if(ans) return ERR_OK;
    return ERR_CANTRUN;
}

static errcodes_t cmd_displco(const char *cmd, char *args){
    int32_t CHno, val;
    if(!argsvals(args, &CHno, &val)) return getta(cmd, CHno, as3935_get_displco);
    errcodes_t ret = senscmd8(CHno, as3935_displco, val);
    if(ret == ERR_OK) DISPLCO[CHno] = val;
    return ret;
}

#define AS3935_FN8(nm) \
static errcodes_t cmd_ ## nm(const char* cmd, char* args){ \
    int32_t CHno = -1, val; \
    if(!argsvals(args, &CHno, &val)) return getta(cmd, CHno, as3935_get_ ## nm); \
    return senscmd8(CHno, as3935_ ## nm, val);}

#define AS3935_FN(nm) \
static errcodes_t cmd_ ## nm(const char*, char* args){ \
        int32_t CHno = -1; \
        splitargs(args, &CHno); \
        return senscmd(CHno, as3935_ ## nm);}

AS3935_FN8(tuncap)
AS3935_FN8(gain)
AS3935_FN8(wdthres)
AS3935_FN8(nflev)
AS3935_FN8(srej)
AS3935_FN8(minnumlig)
AS3935_FN8(maskdist)
AS3935_FN8(lco_fdiv)
AS3935_FN(wakeup)
AS3935_FN(clearstat)
AS3935_FN(resetdef)

#define AS3935_GU8(nm) \
static errcodes_t cmd_ ## nm(const char* cmd, char* args){ \
    int32_t CHno = -1; uint8_t par;\
    splitargs(args, &CHno); \
    if(CHno < 0 || CHno >= SENSORS_AMOUNT) return ERR_BADPAR; \
    CS(CHno); int ans = as3935_ ## nm(&par); CS_OFF(); \
    if(!ans) return ERR_CANTRUN; \
    CMDEQP(CHno); SEND(u2str(par)); SEND("\n"); \
    return ERR_AMOUNT; }

AS3935_GU8(intcode)
AS3935_GU8(distance)

static errcodes_t cmd_energy(const char* cmd, char* args){
    int32_t CHno = -1; uint32_t par;
    splitargs(args, &CHno);
    if(CHno < 0 || CHno >= SENSORS_AMOUNT) return ERR_BADPAR;
    CS(CHno); int ans = as3935_energy(&par); CS_OFF();
    if(!ans) return ERR_CANTRUN;
    CMDEQP(CHno); SEND(u2str(par)); SEND("\n");
    return ERR_AMOUNT;
}

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

static errcodes_t cmd_SPI(const char _U_ *cmd, char *args){
    if(!args) return ERR_BADVAL;
    if(!(SPI1->CR1 & SPI_CR1_SPE)) return ERR_CANTRUN;
    int32_t CHno;
    char *setter = splitargs(args, &CHno);
    if(!setter) return ERR_BADVAL;
    if(CHno < 0 || CHno >= SENSORS_AMOUNT) return ERR_BADPAR;
    int len = parse_hex_data(setter, curbuf, MAXSTRLEN);
    if(len <= 0) return ERR_BADVAL;
    CS(CHno);
    uint8_t ret = SPI_transmit(curbuf, len);
    CS_OFF();
    if(!ret) return ERR_BUSY;
    CMDEQP(CHno);
    if(len > 8) SEND("\n");
    hexdump(SEND, curbuf, len);
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
