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
#include "gpioproto.h"
#include "gpio.h"
#include "gpioproto.h"
#undef USBIF
#define USBIF   IGPIO
#include "strfunc.h"
}

extern volatile uint32_t Tms;

// TODO: add analog threshold!

// list of all commands and handlers
#define COMMAND_TABLE \
    COMMAND(canspeed,   "CAN bus speed setter/getter (kBaud, 10..1000)") \
    COMMAND(curcanspeed,"current CAN bus speed (interface speed, not settings)") \
    COMMAND(dumpconf,   "dump current configuration") \
    COMMAND(eraseflash, "erase full flash storage") \
    COMMAND(help,       "show this help") \
    COMMAND(mcureset,   "reset MCU") \
    COMMAND(PA,         "GPIOA setter/getter (type PA0=help for further info)") \
    COMMAND(PB,         "GPIOB setter/getter") \
    COMMAND(readconf,  "re-read config from flash") \
    COMMAND(reinit,     "apply pin config") \
    COMMAND(saveconf,   "save current user configuration into flash") \
    COMMAND(sendcan,    "send all after '=' to CAN USB interface") \
    COMMAND(setiface,   "set/get name of interface x (0 - CAN, 1 - GPIO)") \
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
    int index; // index in str_keywords
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

static const Keyword keywords[] = {
#define KEY(x, g, v)  { STR_ ## x, g, v},
    KEY(AIN, GROUP_MODE,  MODE_ANALOG)
    KEY(IN, GROUP_MODE,  MODE_INPUT)
    KEY(OUT, GROUP_MODE,  MODE_OUTPUT)
    KEY(AF, GROUP_MODE,  MODE_AF)
    KEY(PU, GROUP_PULL,  PULL_UP)
    KEY(PD, GROUP_PULL,  PULL_DOWN)
    KEY(FL, GROUP_PULL,  PULL_NONE)
    KEY(PP, GROUP_OTYPE, OUTPUT_PP)
    KEY(OD, GROUP_OTYPE, OUTPUT_OD)
    KEY(USART, GROUP_FUNC,  FUNC_USART)
    KEY(SPI, GROUP_FUNC,  FUNC_SPI)
    KEY(I2C, GROUP_FUNC, FUNC_I2C)
    KEY(MONITOR, GROUP_MISC,  MISC_MONITOR)
#undef K
};
#define NUM_KEYWORDS (sizeof(keywords)/sizeof(keywords[0]))

static const char* errtxt[ERR_AMOUNT] = {
    [ERR_OK]       = "OK",
    [ERR_BADCMD]   = "BADCMD",
    [ERR_BADPAR]   = "BADPAR",
    [ERR_BADVAL]   = "BADVAL",
    [ERR_WRONGLEN] = "WRONGLEN",
    [ERR_CANTRUN]  = "CANTRUN",
};

static const char *pinhelp =
    "Pin settings: PXx = MODE PULL OTYPE FUNC MISC (in any sequence), where\n"
    "  MODE: AIN, IN or OUT (analog in, digital in, output)\n"
    "  PULL: PU, PD or FL (pullup, pulldown, no pull - floating)\n"
    "  OTYPE: PP or OD (push-pull or open-drain)\n"
    "  FUNC: USART or SPI (enable alternate function and configure peripheal)\n"
    "  MISC: MONITOR (send data by USB as only state changed)\n\n"
    ;

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
        DBG("next="); DBG(next); DBGNL();
        if(*(++next)) next = omit_spaces(next);
        if(*next == 0) next = NULL;
    }
    DBG("next="); DBG(next); DBGNL();
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

// `port` and `pin` are checked in `parse_pin_command`
// `PAx = ` also printed there
static void pin_getter(uint8_t port, uint8_t pin){
    int16_t val = pin_in(port, pin);
    if(val < 0) SENDn(errtxt[ERR_CANTRUN]);
    SEND(port == 0 ? "PA" : "PB"); SEND(u2str((uint32_t)pin)); SEND(EQ);
    SENDn(u2str((uint32_t)val));
}

// `port` and `pin` are checked in `parse_pin_command`
// set GPIO values (if *setter is 0/1) or configure it
static errcodes_t pin_setter(uint8_t port, uint8_t pin, char *setter){
    char _1st = *setter;
    if(_1st == '0' || _1st == '1'){ // just set/clear pin state; throw out all text after "1"/"0"
        DBG("set pin\n");
        if(pin_out(port, pin, _1st - '0')) return ERR_OK;
        return ERR_CANTRUN;
    }
    if(strncmp(setter, "help", 4) == 0){ // send PIN help
        SENDn(pinhelp);
        return ERR_AMOUNT;
    }
    // complex setter: parse properties
    uint8_t mode_set = 0xFF, pull_set = 0xFF, otype_set = 0xFF, func_set = 0xFF;
    bool monitor = false;
    char *saveptr, *token = strtok_r(setter, " ,", &saveptr);
    while(token){
        size_t i = 0;
        for(; i < NUM_KEYWORDS; i++){
            if(strcmp(token, str_keywords[keywords[i].index]) == 0){
                switch(keywords[i].group){
                case GROUP_MODE:
                    DBG("GROUP_MODE\n");
                    if(mode_set != 0xFF) return ERR_BADVAL; // repeated similar group parameter
                    mode_set = keywords[i].value;
                    break;
                case GROUP_PULL:
                    DBG("GROUP_PULL\n");
                    if(pull_set != 0xFF) return ERR_BADVAL;
                    pull_set = keywords[i].value;
                    break;
                case GROUP_OTYPE:
                    DBG("GROUP_OTYPE\n");
                    if(otype_set != 0xFF) return ERR_BADVAL;
                    otype_set = keywords[i].value;
                    break;
                case GROUP_FUNC:
                    DBG("GROUP_FUNC\n");
                    if(func_set != 0xFF) return ERR_BADVAL;
                    func_set = keywords[i].value;
                    break;
                case GROUP_MISC:
                    DBG("GROUP_MISC\n");
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
    if(!set_pinfunc(port, pin, &curconf)) return ERR_BADVAL;
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
    DBG("args="); DBG(args); DBG(", pin="); DBG(i2str(pin)); DBG(", setter="); DBG(setter); DBGNL();
    if(pin < 0 || pin > 15) return ERR_BADPAR;
    pinconfig_t *pcfg = &the_conf.pinconfig[port][pin]; // just to check if pin can be configured
    if(!pcfg->enable) return ERR_CANTRUN; // prohibited pin
    if(!setter){ // simple getter -> get value and return ERR_AMOUNT as silence
        DBG("Getter\n");
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
static errcodes_t cmd_canspeed(const char *cmd, char *args){
    int32_t S;
    if(argsvals(args, NULL, &S)){
        if(S < CAN_MIN_SPEED || S > CAN_MAX_SPEED) return ERR_BADVAL;
        the_conf.CANspeed = S;
    }
    SEND(cmd); PUTCHAR('='); SENDn(u2str(the_conf.CANspeed));
    return ERR_AMOUNT;
}

static errcodes_t cmd_curcanspeed(const char *cmd, char _U_ *args){
    SEND(cmd); PUTCHAR('='); SENDn(u2str(CAN_getspeed()));
    return ERR_AMOUNT;
}

static errcodes_t cmd_dumpconf(const char _U_ *cmd, char _U_ *args){
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
    SEND("Pin configuration:\n");
    for(int port = 0; port < 2; port++){
        char port_letter = (port == 0) ? 'A' : 'B';
        for(int pin = 0; pin < 16; pin++){
            pinconfig_t *p = &the_conf.pinconfig[port][pin];
            if(!p->enable) continue;
            PUTCHAR('P'); PUTCHAR(port_letter); SEND(i2str(pin)); PUTCHAR('=');
            switch(p->mode){
#define S(k)  SEND(str_keywords[STR_ ## k])
#define SP(k) do{PUTCHAR(' '); S(k);}while(0)
            case MODE_INPUT:
                S(IN);
                if(p->pull == PULL_UP) SP(PU);
                else if (p->pull == PULL_DOWN) SP(PD);
                else SP(FL);
                break;
            case MODE_OUTPUT:
                S(OUT);
                if(p->otype == OUTPUT_PP) SP(PP);
                else SP(OD);
                if(p->pull == PULL_UP) SP(PU);
                else if (p->pull == PULL_DOWN) SP(PD);
                break;
            case MODE_ANALOG:
                S(AIN);
                break;
            case MODE_AF:
                switch(p->af){
                case FUNC_USART: S(USART); break;
                case FUNC_SPI: S(SPI); break;
                case FUNC_I2C: S(I2C); break;
                default: SEND("UNKNOWN_AF");
                }
                break;
            }
            /*
            if(p->mode == MODE_OUTPUT || p->mode == MODE_AF){
                switch(p->speed){
                case SPEED_LOW: SEND(" LOWSPEED"); break;
                case SPEED_MEDIUM: SEND(" MEDSPEED"); break;
                case SPEED_HIGH: SEND(" HIGHSPEED"); break;
                default: break;
                }
            }*/
            // Monitor
            if(p->monitor) SP(MONITOR);
            NL();
#undef S
#undef SP
        }
    }
    // here are usart/spi/i2c configurations
#if 0
    bool usart_enabled = false;
    for (int port = 0; port < 2 && !usart_enabled; port++) {
        for (int pin = 0; pin < 16; pin++) {
            pinconfig_t *p = &the_conf.pinconfig[port][pin];
            if (p->enable && p->mode == MODE_AF && p->af == FUNC_USART) {
                usart_enabled = true;
                break;
            }
        }
    }
    if (usart_enabled) {
        SEND("usart=");
        // usart_config (baud, bits, parity, stopbits)
        // e.g: SEND(u2str(usart_config.baudrate)); SEND(" ");
        // SEND(i2str(usart_config.databits)); PUTCHAR(usart_config.parity); SEND(i2str(usart_config.stopbits));
        NL();
    }
    bool spi_enabled = false;
    for(int port = 0; port < 2 && !spi_enabled; port++){
        for (int pin = 0; pin < 16; pin++) {
            pinconfig_t *p = &the_conf.pinconfig[port][pin];
            if (p->enable && p->mode == MODE_AF && p->af == FUNC_SPI) {
                spi_enabled = true;
                break;
            }
        }
    }
    if (spi_enabled) {
        SEND("spi=");
        // spi_config (speed, mode)
        NL();
    }
#endif
    return ERR_AMOUNT;
}

static errcodes_t cmd_setiface(const char* cmd, char *args){
    int32_t N;
    char *setter = splitargs(args, &N);
    if(N < 0 || N >= InterfacesAmount) return ERR_BADPAR;
    if(setter && *setter){ // setter
        int l = strlen(setter);
        if(l > MAX_IINTERFACE_SZ) return ERR_BADVAL;
        the_conf.iIlengths[N] = (uint8_t) l * 2;
        char *ptr = (char*)the_conf.iInterface[N];
        for(int i = 0; i < l; ++i){
            char c = *setter++;
            *ptr++ = (c > ' ') ? c : '_';
            *ptr++ = 0;
        }
    }
    // getter
    SEND(cmd); PUTCHAR('=');
    char *ptr = (char*) the_conf.iInterface[N];
    int l = the_conf.iIlengths[N] / 2;
    for(int j = 0; j < l; ++j){
        PUTCHAR(*ptr);
        ptr += 2;
    }
    NL();
    return ERR_AMOUNT;
}

static errcodes_t cmd_sendcan(const char _U_ *cmd, char *args){
    if(!args) return ERR_BADVAL;
    char *setter = splitargs(args, NULL);
    if(!setter) return ERR_BADVAL;
    if(USB_sendstr(ICAN, setter)) return ERR_OK;
    return ERR_CANTRUN;
}

static errcodes_t cmd_time(const char *cmd, char _U_ *args){
    SEND(cmd); PUTCHAR('='); SENDn(u2str(Tms));
    return ERR_AMOUNT;
}

static errcodes_t cmd_mcureset(const char _U_ *cmd, char _U_ *args){
    NVIC_SystemReset();
    return ERR_CANTRUN; // never reached
}

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

static const char *cmd_parser(char *str){
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

void GPIO_process(){
    char inbuff[MAXSTRLEN];
    int l = RECV(inbuff, MAXSTRLEN);
    // TODO: check SPI/USART/I2C
    for(uint8_t port = 0; port < 2; ++port){
        uint16_t alert = gpio_alert(port);
        if(alert == 0) continue;
        uint16_t pinmask = 1;
        for(uint8_t i = 0; i < 16; ++i, pinmask <<= 1){
            if(alert & pinmask) pin_getter(port, i);
        }
    }
    if(l == 0) return;
    if(l < 0) SEND("ERROR: USB buffer overflow or string was too long\n");
    else{
        const char *ans = cmd_parser(inbuff);
        if(ans) SENDn(ans);
    }
}
