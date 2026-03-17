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
#include "adc.h"
#include "can.h"
#include "flash.h"
#include "gpio.h"
#include "gpioproto.h"
#include "i2c.h"
#include "pwm.h"
#include "usart.h"
#undef USBIF
#define USBIF   IGPIO
#include "strfunc.h"
}

extern volatile uint32_t Tms;

static uint8_t curbuf[MAXSTRLEN]; // buffer for receiving data from USART etc

static uint8_t usart_text = 0; // ==1 for text USART proto
static uint8_t hex_input_mode = 0; // ==0 for text input, 1 for HEX + text in quotes

// TODO: add analog threshold!

// list of all commands and handlers
#define COMMAND_TABLE \
    COMMAND(canspeed,   "CAN bus speed setter/getter (kBaud, 10..1000)") \
    COMMAND(curcanspeed,"current CAN bus speed (interface speed, not settings)") \
    COMMAND(curpinconf, "dump current (maybe wrong) pin configuration") \
    COMMAND(dumpconf,   "dump global configuration") \
    COMMAND(eraseflash, "erase full flash storage") \
    COMMAND(help,       "show this help") \
    COMMAND(hexinput,   "input is text (0) or hex + text in quotes (1)") \
    COMMAND(iic,        "write data over I2C: I2C=addr data (hex)") \
    COMMAND(iicread,    "I2C read: I2Cread=addr nbytes (hex)") \
    COMMAND(iicreadreg, "I2C read register: I2Creadreg=addr reg nbytes (hex)") \
    COMMAND(iicscan,    "Scan I2C bus for devices") \
    COMMAND(mcutemp,    "get MCU temperature (degC*10)") \
    COMMAND(mcureset,   "reset MCU") \
    COMMAND(PA,         "GPIOA setter/getter (type PA0=help for further info)") \
    COMMAND(PB,         "GPIOB setter/getter") \
    COMMAND(pinout,     "list pinout with all available functions (or selected in setter, like pinout=USART,AIN") \
    COMMAND(pwmmap,     "show pins with PWM ability") \
    COMMAND(readconf,   "re-read config from flash") \
    COMMAND(reinit,     "apply pin config") \
    COMMAND(saveconf,   "save current user configuration into flash") \
    COMMAND(sendcan,    "send all after '=' to CAN USB interface") \
    COMMAND(setiface,   "set/get name of interface x (0 - CAN, 1 - GPIO)") \
    COMMAND(storeconf,  "save config to flash") \
    COMMAND(time,       "show current time (ms)") \
    COMMAND(USART,      "Read USART data or send (USART=hex)") \
    COMMAND(vdd,        "get approx Vdd value (V*100)")
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
    MISC_THRESHOLD,
    MISC_SPEED,
    MISC_TEXT,
    MISC_HEX
};

// TODO: add HEX input?

#define KEYWORDS \
    KW(AIN) \
    KW(IN) \
    KW(OUT) \
    KW(AF) \
    KW(PU)\
    KW(PD) \
    KW(FL) \
    KW(PP) \
    KW(OD) \
    KW(USART) \
    KW(SPI) \
    KW(I2C) \
    KW(MONITOR) \
    KW(THRESHOLD) \
    KW(SPEED) \
    KW(TEXT) \
    KW(HEX) \
    KW(PWM) \


typedef enum{ // indexes of string keywords
#define KW(k) STR_ ## k,
        KEYWORDS
#undef KW
} kwindex_t;

// strings for keywords
static const char *str_keywords[] = {
#define KW(x)    [STR_ ## x] = #x,
    KEYWORDS
#undef KW
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
    KEY(PWM, GROUP_FUNC, FUNC_PWM)
    KEY(MONITOR, GROUP_MISC,  MISC_MONITOR)
    KEY(THRESHOLD, GROUP_MISC,  MISC_THRESHOLD)
    KEY(SPEED, GROUP_MISC, MISC_SPEED)
    KEY(TEXT, GROUP_MISC, MISC_TEXT)
    KEY(HEX, GROUP_MISC, MISC_HEX)
#undef K
};
#define NUM_KEYWORDS (sizeof(keywords)/sizeof(keywords[0]))

static const char* errtxt[ERR_AMOUNT] = {
    [ERR_OK]        = "OK",
    [ERR_BADCMD]    = "BADCMD",
    [ERR_BADPAR]    = "BADPAR",
    [ERR_BADVAL]    = "BADVAL",
    [ERR_WRONGLEN]  = "WRONGLEN",
    [ERR_CANTRUN]   = "CANTRUN",
    [ERR_BUSY]      = "BUSY",
    [ERR_OVERFLOW]  = "OVERFLOW",
};

static const char *pinhelp =
    "Pin settings: PXx = MODE PULL OTYPE FUNC MISC (in any sequence), where\n"
    "  MODE: AIN, IN or OUT (analog in, digital in, output), also AF (automatically set when AF selected)\n"
    "  PULL: PU, PD or FL (pullup, pulldown, no pull - floating)\n"
    "  OTYPE: PP or OD (push-pull or open-drain)\n"
    "  FUNC: USART, SPI, I2C or PWM (enable alternate function and configure peripheal)\n"
    "  MISC: MONITOR - send data by USB as only state changed\n"
    "        THRESHOLD (ADC only) - monitoring threshold, ADU\n"
    "        SPEED - interface speed/frequency\n"
    "        TEXT - USART means data as text ('\\n'-separated strings)\n"
    "        HEX - USART means data as binary (output: HEX)\n"
    "\n"
    ;

static const char *EQ = " = "; // equal sign for getters
// token delimeters in setters
static const char *DELIM_ = " ,";
static const char *COMMA = ", "; // comma before next val in list

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

// `port` and `pin` are checked in `parse_pin_command`
// `PAx = ` also printed there
static void pin_getter(uint8_t port, uint8_t pin){
    int16_t val = pin_in(port, pin);
    if(val < 0){
        SENDn(errtxt[ERR_CANTRUN]);
        return;
    }
    SEND(port == 0 ? "PA" : "PB"); SEND(u2str((uint32_t)pin)); SEND(EQ);
    SENDn(u2str((uint32_t)val));
}

// `port` and `pin` are checked in `parse_pin_command`
// set GPIO values (if *setter is 0/1) or configure it
static errcodes_t pin_setter(uint8_t port, uint8_t pin, char *setter){
    if(strncmp(setter, "help", 4) == 0){ // send PIN help
        SENDn(pinhelp);
        return ERR_AMOUNT;
    }
    pinconfig_t curconf;
    if(!get_curpinconf(port, pin, &curconf)) return ERR_BADVAL; // copy current config
    uint32_t U32;
    char *end = getnum(setter, &U32);
    if(end != setter && *end == 0){ // number -> set pin/PWM value
        if(U32 > 0xff) return ERR_BADVAL;
        uint8_t val = (uint8_t) U32;
        if(curconf.mode == MODE_OUTPUT){ // set/clear pin
            if(U32 > 1) U32 = 1;
            DBG("set pin\n");
            if(pin_out(port, pin, val)) return ERR_OK;
            return ERR_CANTRUN;
        }else if(curconf.mode == MODE_AF && curconf.af == FUNC_PWM){
            if(setPWM(port, pin, val)) return ERR_OK;
            return ERR_CANTRUN;
        }
    }
    // complex setter: parse properties
    uint8_t mode_set = 0xFF, pull_set = 0xFF, otype_set = 0xFF, func_set = 0xFF;
    bool monitor = false;
    uint16_t *pending_u16 = NULL; // pointer to uint16_t value, if !NULL, next token should be a number
    uint32_t *pending_u32 = NULL; // -//- for uint32_t
    usartconf_t UsartConf;
    if(!get_curusartconf(&UsartConf)) return ERR_CANTRUN;
    char *saveptr, *token = strtok_r(setter, DELIM_, &saveptr);
    while(token){
        if(pending_u16){
            uint32_t val;
            end = getnum(token, &val);
            if(end == token || val > 0xFFFF) return ERR_BADVAL;
            *pending_u16 = (uint16_t)val;
            pending_u16 = NULL;  // reset
            token = strtok_r(NULL, DELIM_, &saveptr);
            continue;
        }
        if(pending_u32){
            uint32_t val;
            end = getnum(token, &val);
            if(end == token) return ERR_BADVAL;
            *pending_u32 = val;
            pending_u32 = NULL;
            token = strtok_r(NULL, DELIM_, &saveptr);
            continue;
        }
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
                    switch(keywords[i].value){
                    case MISC_MONITOR:
                        monitor = true;
                        break;
                    case MISC_THRESHOLD:
                        pending_u16 = &curconf.threshold;
                        break;
                    case MISC_SPEED:
                        pending_u32 = &UsartConf.speed; // also used for I2C speed!
                        break;
                    case MISC_TEXT: // what to do, if textproto is set, but user wants binary?
                        UsartConf.textproto = 1;
                        break;
                    case MISC_HEX: // clear text flag
                        UsartConf.textproto = 0;
                        break;
                    }
                    break;
                }
                break;
            }
        }
        if(i == NUM_KEYWORDS) return ERR_BADVAL; // not found
        token = strtok_r(NULL, DELIM_, &saveptr);
    }
    if(pending_u16 || pending_u32) return ERR_BADVAL; // no number that we waiting for

// check periferial settings before refresh pin data
    // check current USART settings
    if(func_set == FUNC_USART && !chkusartconf(&UsartConf)) return ERR_BADVAL;
    if(func_set == FUNC_I2C){ // check speed
        i2c_speed_t s = (UsartConf.speed > I2C_SPEED_1M) ? I2C_SPEED_10K : static_cast <i2c_speed_t> (UsartConf.speed);
        the_conf.I2Cspeed = static_cast <uint8_t> (s);
    }
    if(func_set != 0xFF) mode_set = MODE_AF;
    if(mode_set == 0xFF) return ERR_BADVAL; // user forgot to set mode
    // set defaults
    if(pull_set == 0xFF) pull_set = PULL_NONE;
    if(otype_set == 0xFF) otype_set = OUTPUT_PP;
    // can also do something with `speed_set`, then remove SPEED_MEDIUM from `curconfig`
    // check that current parameters combination is acceptable for current pin
    curconf.mode = static_cast <pinmode_t> (mode_set);
    curconf.pull = static_cast <pinpull_t> (pull_set);
    curconf.otype = static_cast <pinout_t> (otype_set);
    curconf.speed = SPEED_MEDIUM;
    curconf.af = static_cast <funcnames_t> (func_set);
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
    if(is_disabled(port, pin)) return ERR_CANTRUN; // prohibited pin
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
    if(gpio_reinit()){
        usartconf_t UC;
        if(get_curusartconf(&UC)){
            usart_text = UC.textproto;
        }
        return ERR_OK;
    }
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
    CMDEQ();
    SENDn(u2str(the_conf.CANspeed));
    return ERR_AMOUNT;
}

static errcodes_t cmd_curcanspeed(const char *cmd, char _U_ *args){
    CMDEQ();
    SENDn(u2str(CAN_getspeed()));
    return ERR_AMOUNT;
}

// dump global pin config (current == 0) or current (==1)
static void dumppinconf(int current){
    if(current) SEND("Current p");
    else PUTCHAR('P');
    SEND("in configuration:\n");
#define S(k)  SEND(str_keywords[STR_ ## k])
#define SP(k) do{PUTCHAR(' '); S(k);}while(0)
    for(int port = 0; port < 2; port++){
        char port_letter = (port == 0) ? 'A' : 'B';
        for(int pin = 0; pin < 16; pin++){
            pinconfig_t cur, *p = &cur;
            if(current && !get_curpinconf(port, pin, &cur)) continue; // local
            if(!current) p = &the_conf.pinconfig[port][pin]; // global
            if(!p->enable) continue;
            PUTCHAR('P'); PUTCHAR(port_letter); SEND(i2str(pin)); SEND(EQ);
            switch(p->mode){
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
                if(p->threshold){
                    SP(THRESHOLD);
                    PUTCHAR(' ');
                    SEND(u2str(p->threshold));
                }
                break;
            case MODE_AF:
                switch(p->af){
                case FUNC_USART: S(USART); break;
                case FUNC_SPI: S(SPI); break;
                case FUNC_I2C: S(I2C); break;
                case FUNC_PWM: S(PWM); break;
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
        }
    }
#undef S
#undef SP
}

static errcodes_t cmd_curpinconf(const char _U_ *cmd, char _U_ *args){
    dumppinconf(TRUE);
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
    dumppinconf(FALSE); // global pin config
#define S(k)  SEND(str_keywords[STR_ ## k])
#define SP(k) do{PUTCHAR(' '); S(k);}while(0)
    usartconf_t U = the_conf.usartconfig;
    if(U.RXen || U.TXen){ // USART enabled -> tell config
        S(USART); SEND(EQ);
        S(SPEED); PUTCHAR(' '); SEND(u2str(U.speed));
        if(U.textproto) SP(TEXT);
        if(U.monitor) SP(MONITOR);
        if(U.TXen && !U.RXen) SEND(" TXONLY");
        else if(!U.TXen && U.RXen) SEND(" RXONLY");
        NL();
    }
    if(I2C1->CR1 & I2C_CR1_PE){ // I2C active, show its speed
        SEND("iicspeed=");
        switch(the_conf.I2Cspeed){
        case 0: SEND("10kHz"); break;
        case 1: SEND("100kHz"); break;
        case 2: SEND("400kHz"); break;
        case 3: SEND("1MHz"); break;
        default: SEND("unknown");
        }
        NL();
    }
#undef S
#undef SP
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
    CMDEQP(N);
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
    if(hex_input_mode){
        int len = parse_hex_data(setter, curbuf, MAXSTRLEN);
        if(len < 0) return ERR_BADVAL;
        if(len == 0) return ERR_AMOUNT;
        if(USB_send(ICAN, curbuf, len)) return ERR_OK;
    }else{
        if(USB_sendstr(ICAN, setter)){
            USB_putbyte(ICAN, '\n');
            return ERR_OK;
        }
    }
    return ERR_CANTRUN;
}

static errcodes_t cmd_time(const char *cmd, char _U_ *args){
    CMDEQ();
    SENDn(u2str(Tms));
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

static errcodes_t cmd_mcutemp(const char *cmd, char _U_ *args){
    CMDEQ();
    SENDn(i2str(getMCUtemp()));
    return ERR_AMOUNT;
}

static errcodes_t cmd_vdd(const char *cmd, char _U_ *args){
    CMDEQ();
    SENDn(u2str(getVdd()));
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

static errcodes_t cmd_hexinput(const char *cmd, char *args){
    int32_t val;
    if(argsvals(args, NULL, &val)){
        if(val == 0 || val == 1) hex_input_mode = (uint8_t)val;
        else return ERR_BADVAL;
    }
    CMDEQ();
    SENDn(hex_input_mode ? "1" : "0");
    return ERR_AMOUNT;
}

static errcodes_t cmd_pwmmap(const char _U_ *cmd, char _U_ *args){
    pwmtimer_t t;
    SEND("PWM pins:\n");
    for(int port = 0; port < 2; ++port){
        for(int pin = 0; pin < 16; ++pin){
            if(!canPWM(port, pin, &t)) continue;
            SEND((port == 0) ? "PA" : "PB");
            SEND(u2str(pin));
            if(t.collision){
                SEND(" conflicts with ");
                SEND((t.collport == 0) ? "PA" : "PB");
                SEND(u2str(t.collpin));
            }
            NL();
        }
    }
    return ERR_AMOUNT;
}

static int sendfun(const char *s){
    if(!s) return 0;
    return USB_sendstr(IGPIO, s);
}

static void sendusartdata(const uint8_t *buf, int len){
    if(!buf || len < 1) return;
    SEND(str_keywords[STR_USART]); SEND(EQ);
    if(usart_text){
        USB_send(IGPIO, curbuf, len);
        NL(); // always add newline at the end to mark real newline ("\n\n") and piece of line ("\n")
    }else{
        NL();
        hexdump(sendfun, (uint8_t*)curbuf, len);
    }
}

static errcodes_t cmd_USART(const char _U_ *cmd, char *args){
    if(!args) return ERR_BADVAL;
    char *setter = splitargs(args, NULL);
    if(setter){
        DBG("Try to send over USART\n");
        if(hex_input_mode){
            int len = parse_hex_data(setter, curbuf, MAXSTRLEN);
            if(len < 0) return ERR_BADVAL;
            if(len > 0) return usart_send(curbuf, len);
        }else{ // text mode: "AS IS"
            int l = strlen(setter);
            if(usart_text){ // add '\n' as we removed it @ parser
                setter[l++] = '\n';
            }
            return usart_send((uint8_t*)setter, l);
        }
    } // getter: try to read
    int l = usart_receive(curbuf, MAXSTRLEN);
    if(l < 0) return ERR_CANTRUN;
    if(l > 0) sendusartdata(curbuf, l);
    // or silence: nothing to read
    return ERR_AMOUNT;
}

static errcodes_t cmd_iic(const char _U_ *cmd, char *args){
    if(!(I2C1->CR1 & I2C_CR1_PE)) return ERR_CANTRUN;
    if(!args) return ERR_BADVAL;
    char *setter = splitargs(args, NULL);
    if(!setter) return ERR_BADVAL;
    int len = parse_hex_data(setter, curbuf, MAXSTRLEN);
    if(len < 1) return ERR_BADVAL; // need at least address
    uint8_t addr = curbuf[0];
    if(addr > 0x7F) return ERR_BADVAL; // 7-битный адрес
    if(len == 1){ // only address without data
        return ERR_BADPAR;
    }
    addr <<= 1; // roll address to run i2c_write
    if(!i2c_write(addr, curbuf + 1, len - 1)){ // len = address + data length
        return ERR_CANTRUN;
    }
    return ERR_OK;
}

static errcodes_t cmd_iicread(const char *cmd, char *args){
    if(!(I2C1->CR1 & I2C_CR1_PE)) return ERR_CANTRUN;
    if(!args) return ERR_BADVAL;
    char *setter = splitargs(args, NULL);
    if(!setter) return ERR_BADVAL;
    int len = parse_hex_data(setter, curbuf, MAXSTRLEN);
    if(len != 2) return ERR_BADVAL; // address, amount of bytes
    uint8_t addr = curbuf[0];
    uint8_t nbytes = curbuf[1];
    if(addr > 0x7F) return ERR_BADVAL; // allow to "read" 0 bytes (just get ACK)
    addr <<= 1;
    if(!i2c_read(addr, curbuf, nbytes)) return ERR_CANTRUN;
    CMDEQ();
    if(nbytes < 9) NL();
    hexdump(sendfun, curbuf, nbytes);
    return ERR_AMOUNT;
}

static errcodes_t cmd_iicreadreg(const char *cmd, char *args){
    if(!(I2C1->CR1 & I2C_CR1_PE)) return ERR_CANTRUN;
    if(!args) return ERR_BADVAL;
    char *setter = splitargs(args, NULL);
    if(!setter) return ERR_BADVAL;
    int len = parse_hex_data(setter, curbuf, MAXSTRLEN);
    if(len != 3) return ERR_BADVAL; // address, register, amount of bytes
    uint8_t addr = curbuf[0];
    uint8_t nreg = curbuf[1];
    uint8_t nbytes = curbuf[2];
    if(addr > 0x7F || nbytes == 0) return ERR_BADVAL;
    addr <<= 1;
    if(!i2c_read_reg(addr, nreg, curbuf, nbytes)) return ERR_CANTRUN;
    CMDEQ();
    if(nbytes < 9) NL();
    hexdump(sendfun, curbuf, nbytes);
    return ERR_AMOUNT;
}

static errcodes_t cmd_iicscan(const char _U_ *cmd, char _U_ *args){
    if(!(I2C1->CR1 & I2C_CR1_PE)) return ERR_CANTRUN;
    i2c_init_scan_mode();
    return ERR_OK;
}

// array for `cmd_pinout`
static kwindex_t func_array[FUNC_AMOUNT] = {
    [FUNC_AIN]      = STR_AIN,
    [FUNC_USART]    = STR_USART,
    [FUNC_SPI]      = STR_SPI,
    [FUNC_I2C]      = STR_I2C,
    [FUNC_PWM]      = STR_PWM,
};

static errcodes_t cmd_pinout(const char _U_ *cmd, char *args){
    uint8_t listmask = 0xff; // bitmask for funcnames_t
    if(args && *args){ // change listmask by user choise
        char *setter = splitargs(args, NULL);
        if(!setter) return ERR_BADVAL;
        char *saveptr, *token = strtok_r(setter, DELIM_, &saveptr);
        listmask = 0;
        while(token){
            int i = 0;
            for(; i < FUNC_AMOUNT; ++i){
                if(0 == strcmp(token, str_keywords[func_array[i]])){
                    listmask |= (1 << i);
                    break;
                }
            }
            if(i == FUNC_AMOUNT) return ERR_BADVAL; // wrong argument
            token = strtok_r(NULL, DELIM_, &saveptr);
        }
        if(listmask == 0) return ERR_BADVAL;
    }
    pwmtimer_t tp;      // timers' pins
    usart_props_t up;   // USARTs' pins
    i2c_props_t ip;     // I2C's pins

    SEND("\nConfigurable pins (check collisions if functions have same name!):\n");
    for(int port = 0; port < 2; ++port){
        for(int pin = 0; pin < 16; ++pin){
            int funcs = pinfuncs(port, pin);
            if(funcs == -1) continue;
            uint8_t mask = (static_cast <uint8_t> (funcs)) & listmask;
            if(listmask != 0xff && !mask) continue; // no asked functions
            SEND((port == 0) ? "PA" : "PB");
            SEND(u2str(pin));
            SEND(": ");
            if(listmask == 0xff) SEND("GPIO"); // don't send "GPIO" for specific choice
            if(mask & (1 << FUNC_AIN)){ SEND(COMMA); SEND(str_keywords[STR_AIN]); }
            if(mask & (1 << FUNC_USART)){ // USARTn_aX (n - 1/2, a - R/T)
                SEND(", ");
                int idx = get_usart_index(port, pin, &up);
                SEND(str_keywords[STR_USART]); PUTCHAR('1' + idx);
                PUTCHAR('_'); PUTCHAR(up.isrx ? 'R' : 'T'); PUTCHAR('X');
            }
            if(mask & (1 << FUNC_SPI)){
                SEND(COMMA); SEND(str_keywords[STR_SPI]);
                // TODO: MISO/MOSI/SCL
            }
            if(mask & (1 << FUNC_I2C)){
                int idx = get_i2c_index(port, pin, &ip);
                SEND(COMMA); SEND(str_keywords[STR_I2C]); PUTCHAR('1' + idx);
                PUTCHAR('_');
                SEND(ip.isscl ? "SCL" : "SDA");
            }
            if(mask & (1 << FUNC_PWM)){
                canPWM(port, pin, &tp);
                SEND(COMMA); SEND(str_keywords[STR_PWM]);
                SEND(u2str(tp.timidx)); // timidx == TIMNO!
                PUTCHAR('_');
                PUTCHAR('1' + tp.chidx);
            }
            NL();
        }
    }
    return ERR_AMOUNT;
}

constexpr uint32_t hash(const char* str, uint32_t h = 0){
    return *str ? hash(str + 1, h + ((h << 7) ^ *str)) : h;
}

static const char *CommandParser(char *str){
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
    int l;
    // TODO: check SPI/I2C etc
    for(uint8_t port = 0; port < 2; ++port){
        uint16_t alert = gpio_alert(port);
        if(alert == 0) continue;
        uint16_t pinmask = 1;
        for(uint8_t i = 0; i < 16; ++i, pinmask <<= 1){
            if(alert & pinmask) pin_getter(port, i);
        }
    }
    l = usart_process(curbuf, MAXSTRLEN);
    if(l > 0) sendusartdata(curbuf, l);
    l = RECV((char*)curbuf, MAXSTRLEN);
    if(l){
        if(l < 0) SEND("ERROR: USB buffer overflow or string was too long\n");
        else{
            const char *ans = CommandParser((char*)curbuf);
            if(ans) SENDn(ans);
        }
    }
    if(I2C_scan_mode){
        uint8_t addr;
        if(i2c_scan_next_addr(&addr)){
            SEND("foundaddr = ");
            printuhex(addr);
            NL();
        }
    }
}

// starting init by flash settings
void GPIO_init(){
    gpio_reinit();
    pwm_setup();
    usartconf_t usc;
    if(get_curusartconf(&usc)) usart_text = usc.textproto;
}
