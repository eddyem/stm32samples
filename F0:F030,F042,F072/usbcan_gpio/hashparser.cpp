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

// !!! Some commands could change icoming string, so don't try to use it after function call !!!

#include <stdint.h>

extern "C"{
#include <stm32f0.h>
#include "can.h"
#include "flash.h"
#include "hashparser.h"
#include "gpio.h"
#include "gpioproto.h"
#define USBIF   IGPIO
#include "strfunc.h"
}

extern uint32_t Tms;

// list of all commands and handlers
#define COMMAND_TABLE \
    COMMAND(canspeed,   "CAN bus speed setter/getter (kBaud, 10..1000)") \
    COMMAND(dumpflash,  "flash config dump") \
    COMMAND(help,       "Show this help") \
    COMMAND(PA,         "GPIOA setter/getter (type PAx=help for further info)") \
    COMMAND(PB,         "GPIOB setter/getter") \
    COMMAND(reinit,     "apply pin config") \
    COMMAND(storeconf,  "save config to flash") \
    COMMAND(time,       "show current time (ms)")

//    COMMAND(USART,      "Read USART data or send (USART=hex)")
//    COMMAND(usartconf,  "set USART params (e.g. usartconf=115200 8N1)")
//    COMMAND(SPI,        "Read SPI data or send (SPI=hex)")
//    COMMAND(spiconf,    "set SPI params")


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

// pin settings parser
struct Keyword {
    const char *name;
    uint8_t group;
    uint8_t value;
};

enum KeywordGroup {
    GROUP_MODE,
    GROUP_PULL,
    GROUP_OTYPE,
    GROUP_FUNC,
    GROUP_MISC
};

enum MiscValues{
    MISC_MONITOR = 1,
};

static constexpr Keyword keywords[] = {
    {"AIN",    GROUP_MODE,  MODE_ANALOG},
    {"IN",     GROUP_MODE,  MODE_INPUT},
    {"OUT",    GROUP_MODE,  MODE_OUTPUT},
    {"AF",     GROUP_MODE,  MODE_AF},
    {"PU",     GROUP_PULL,  PULL_UP},
    {"PD",     GROUP_PULL,  PULL_DOWN},
    {"FL",     GROUP_PULL,  PULL_NONE},
    {"PP",     GROUP_OTYPE, OUTPUT_PP},
    {"OD",     GROUP_OTYPE, OUTPUT_OD},
    {"USART",  GROUP_FUNC,  FUNC_USART},
    {"SPI",    GROUP_FUNC,  FUNC_SPI},
    {"MONITOR",GROUP_MISC,  MISC_MONITOR}, // monitor flag
};
#define NUM_KEYWORDS (sizeof(keywords)/sizeof(keywords[0]))

static const char* errtxt[ERR_AMOUNT] = {
    [ERR_OK] =   "OK",
    [ERR_BADCMD] = "BADCMD",
    [ERR_BADPAR] =  "BADPAR",
    [ERR_BADVAL] = "BADVAL",
    [ERR_WRONGLEN] = "WRONGLEN",
    [ERR_CANTRUN] = "CANTRUN",
};

static const char *EQ = " = "; // equal sign for getters

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
        if(*(++next) == 0) next = NULL;
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
    if(next != setter && parval){
        *parval = I32;
        return true;
    }
    return false;
}
#endif

// `port` and `pin` are checked in `parse_pin_command`
// `PAx = ` also printed there
static void pin_getter(uint8_t port, uint8_t pin){
    pinconfig_t *pcfg = &the_conf.pinconfig[port][pin];
    switch(pcfg->mode){
    case MODE_INPUT:
    case MODE_OUTPUT: {
        uint32_t idr = (port == 0) ? GPIOA->IDR : GPIOB->IDR;
        uint8_t bit = (idr >> pin) & 1;
        PUTCHAR(bit ? '1' : '0');
        NL();
    }
        break;
    case MODE_ANALOG:
        SENDn("TODO");
        // TODO: read ADC channel #pin
        //SENDn(u2str(get_adc_value(port, pin)));
        break;
    case MODE_AF:
        SENDn("ERR: pin in AF mode, use USART/SPI commands");
        break;
    default:
        break;
    }
}

// `port` and `pin` are checked in `parse_pin_command`
// set GPIO values (if *setter is 0/1) or configure it
static errcodes_t pin_setter(uint8_t port, uint8_t pin, char *setter){
    pinconfig_t *pcfg = &the_conf.pinconfig[port][pin];
    char _1st = *setter;
    if(_1st == '0' || _1st == '1'){ // just set/clear pin state; throw out all text after "1"/"0"
        if(pcfg->mode != MODE_OUTPUT) return ERR_CANTRUN;
        volatile GPIO_TypeDef * GPIOx = (port == 0) ? GPIOA : GPIOB;
        if(_1st == '1') GPIOx->BSRR = (1 << pin);
        else GPIOx->BRR = (1 << pin);
        return ERR_OK;
    }
    // complex setter: parse properties
    uint8_t mode_set = 0xFF, pull_set = 0xFF, otype_set = 0xFF, func_set = 0xFF;
    bool monitor = false;
    char *saveptr, *token = strtok_r(setter, " ,", &saveptr);
    while(token){
        size_t i = 0;
        for(; i < NUM_KEYWORDS; i++){
            if(strcmp(token, keywords[i].name) == 0){
                switch(keywords[i].group){
                case GROUP_MODE:
                    if(mode_set != 0xFF) return ERR_BADVAL; // repeated similar group parameter
                    mode_set = keywords[i].value;
                    break;
                case GROUP_PULL:
                    if(pull_set != 0xFF) return ERR_BADVAL;
                    pull_set = keywords[i].value;
                    break;
                case GROUP_OTYPE:
                    if(otype_set != 0xFF) return ERR_BADVAL;
                    otype_set = keywords[i].value;
                    break;
                case GROUP_FUNC:
                    if(func_set != 0xFF) return ERR_BADVAL;
                    func_set = keywords[i].value;
                    break;
                case GROUP_MISC:
                    if(keywords[i].value == MISC_MONITOR) monitor = true;
                    break;
                }
                break;
            }
        }
        if(i == NUM_KEYWORDS) return ERR_BADVAL; // not found
        token = strtok_r(NULL, " ,", &saveptr);
    }
    if(func_set != 0xFF) mode_set = MODE_AF;
    if(mode_set == 0xFF) return ERR_BADVAL; // user forgot to set mode
    // set defaults
    if(pull_set == 0xFF) pull_set = PULL_NONE;
    if(otype_set == 0xFF) otype_set = OUTPUT_PP;
    // can also do something with `speed_set`, then remove SPEED_MEDIUM from `curconfig`
    // check that current parameters combination is acceptable for current pin
    pinconfig_t curconf;
    curconf.mode = static_cast <pinmode_t> (mode_set);
    curconf.pull = static_cast <pinpull_t> (pull_set);
    curconf.otype = static_cast <pinout_t> (otype_set);
    curconf.speed = SPEED_MEDIUM;
    curconf.af = func_set;
    curconf.monitor = monitor;
    if(!is_func_allowed(port, pin, &curconf)) return ERR_BADVAL;
    *pcfg = curconf;
    return ERR_OK;
}

// PAx [= aa], PBx [= bb]
static errcodes_t parse_pin_command(const char *cmd, char *args){
    if(!args) return ERR_BADPAR; // or maybe add list for all pins?
    char port_char = cmd[1];
    if(port_char != 'A' && port_char != 'B') return ERR_BADCMD;
    uint8_t port = (port_char == 'A') ? 0 : 1;
    int32_t pin = -1;
    char *setter = splitargs(args, &pin);
    if(pin < 0 || pin > 15) return ERR_BADPAR;
    pinconfig_t *pcfg = &the_conf.pinconfig[port][pin]; // just to check if pin can be configured
    if(!pcfg->enable) return ERR_CANTRUN; // prohibited pin
    if(!setter){ // simple getter -> get value and return ERR_AMOUNT as silence
        SEND(cmd); SEND(u2str((uint32_t)pin)); SEND(EQ);
        pin_getter(port, pin);
        return ERR_AMOUNT;
    }
    return pin_setter(port, pin, setter);
}

static errcodes_t cmd_PA(const char *cmd, char *args){
    return parse_pin_command(cmd, args);
}
static errcodes_t cmd_PB(const char *cmd, char *args){
    return parse_pin_command(cmd, args);
}

static errcodes_t cmd_reinit(const char _U_ *cmd, char _U_ *args){
    if(gpio_reinit()) return ERR_OK;
    SEND("Can't reinit: check your configuration!\n");
    return ERR_AMOUNT;
}

static errcodes_t cmd_storeconf(const char _U_ *cmd, char _U_ *args){
    if(!store_userconf()) return ERR_CANTRUN;
    return ERR_OK;
}

// canspeed = baudrate (kBaud)
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
