/*
 * This file is part of the nitrogen project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "hardware.h"
#include "i2c.h"
#include "proto.h"
#include "strfunc.h"
#include "version.inc"

#include <stm32f3.h>
#include <string.h>

static uint8_t I2Caddress = 0;

// parno - number of parameter (or -1); cargs - string with arguments (after '=') (==NULL for getter), iarg - integer argument
static int goodstub(const char _U_ *cmd, int _U_ parno, const char _U_ *carg, int32_t _U_ iarg){
    return RET_GOOD;
}

static void sendkey(const char *cmd, int parno, int32_t i){
    USB_sendstr(cmd);
    if(parno > -1) USB_sendstr(u2str((uint32_t)parno));
    USB_putbyte('='); USB_sendstr(i2str(i)); newline();
}
static void sendkeyf(const char *cmd, int parno, float f){
    USB_sendstr(cmd);
    if(parno > -1) USB_sendstr(u2str((uint32_t)parno));
    USB_putbyte('='); USB_sendstr(float2str(f, 2)); newline();
}
static void sendkeyu(const char *cmd, int parno, uint32_t u){
    USB_sendstr(cmd);
    if(parno > -1) USB_sendstr(u2str((uint32_t)parno));
    USB_putbyte('='); USB_sendstr(u2str(u)); newline();
}

static int leds(const char *cmd, int parno, const char *c, int32_t i){
    if(parno < 0){ // enable/disable all
        if(c){
            LEDsON = (i ? 1 : 0);
            if(!LEDsON) for(int _ = 0; _ < 4; ++_) LED_off(_);
        }
        sendkey("LEDon", -1, LEDsON);
    }else{
        if(parno >= LEDS_AMOUNT) return RET_WRONGARG;
        if(c) switch(i){
            case 2: LED_blink(parno); break;
            case 1: LED_on(parno); break;
            default: LED_off(parno);
        }
        sendkey(cmd, parno, LED_get(parno));
    }
    return RET_GOOD;
}

static int buzzer(const char *cmd, int _U_ parno, const char _U_ *c, int32_t i){
    if(c){
        if(i > 0) BUZZER_ON();
        else BUZZER_OFF();
    }
    sendkey(cmd, -1, BUZZER_STATE());
    return RET_GOOD;
}

static int i2scan(const char _U_ *cmd, int _U_ parno, const char _U_ *c, int32_t _U_ i){
    i2c_init_scan_mode();
    return RET_GOOD;
}
static int i2addr(const char *cmd, int _U_ parno, const char *c, int32_t i){
    if(c){
        if(i < 0 || i>= I2C_ADDREND) return RET_WRONGARG;
        I2Caddress = (uint8_t) i;
    }
    sendkey(cmd, -1, I2Caddress);
    return RET_GOOD;
}

static int adcon(const char *cmd, int _U_ parno, const char *c, int32_t i){
    if(c) ADCON(i);
    sendkey(cmd, -1, ADCONSTATE());
    return RET_GOOD;
}
static int adcval(const char *cmd, int parno, const char _U_ *c, int32_t i){
    if(parno >= NUMBER_OF_ADC_CHANNELS) return RET_WRONGPARNO;
    if(parno < 0){ // all channels
        for(i = 0; i < NUMBER_OF_ADC_CHANNELS; ++i) sendkey(cmd, i, getADCval(i));
    }else
        sendkey(cmd, parno, getADCval(parno));
    return RET_GOOD;
}
static int adcvoltage(const char *cmd, int parno, const char _U_ *c, int32_t i){
    if(parno >= NUMBER_OF_ADC_CHANNELS) return RET_WRONGPARNO;
    if(parno < 0){ // all channels
        for(i = 0; i < NUMBER_OF_ADC_CHANNELS; ++i) sendkeyf(cmd, i, getADCvoltage(i));
    }else
        sendkeyf(cmd, parno, getADCvoltage(parno));
    return RET_GOOD;
}
static int mcut(const char *cmd, int _U_ parno, const char _U_ *c, int32_t _U_ i){
    sendkeyf(cmd, -1, getMCUtemp());
    return RET_GOOD;
}

static int reset(const char _U_ *cmd, int _U_ parno, const char _U_ *c, int32_t _U_ i){
    USB_sendstr("RESET!!!\n");
    USB_sendall();
    NVIC_SystemReset();
    return RET_GOOD; // never reached
}

static int tms(const char _U_ *cmd, int _U_ parno, const char _U_ *c, int32_t _U_ i){
    sendkeyu(cmd, -1, Tms);
    return RET_GOOD;
}

static int pwm(const char *cmd, int parno, const char *c, int32_t i){
    if(parno < 0 || parno > 3) return RET_WRONGPARNO;
    if(c) setPWM(parno, (uint8_t)i);
    sendkeyu(cmd, -1, getPWM(parno));
    return RET_GOOD;
}

typedef struct{
    int (*fn)(const char*, int, const char*, int32_t);
    const char *cmd;
    const char *help;
} commands;

commands cmdlist[] = {
    {goodstub, "stub", "simple stub"},
    {NULL, "Different commands", NULL},
    {buzzer, "buzzer", "get/set (0 - off, 1 - on) buzzer"},
    {leds, "LED", "LEDx=y; where x=0..3 to work with single LED (then y=1-set, 0-reset, 2-toggle), absent to work with all (y=0 - disable, 1-enable)"},
    {pwm, "pwm", "set/get x channel (0..3) pwm value (0..255)"},
    {reset, "reset", "reset MCU"},
    {tms, "tms", "print Tms"},
    {NULL, "I2C commands", NULL},
    {i2addr, "iicaddr", "set/get I2C address"},
    {i2scan, "iicscan", "scan I2C bus"},
    {NULL, "ADC commands", NULL},
    {adcval, "ADC", "get ADCx value (without x - for all)"},
    {adcvoltage, "ADCv", "get ADCx voltage (without x - for all)"},
    {mcut, "mcut", "get MCU temperature"},
    {adcon, "sensv", "turn on (1) or off (0) Tsens voltage"},
    {NULL, NULL, NULL}
};

static void printhelp(){
    commands *c = cmdlist;
    USB_sendstr("https://github.com/eddyem/stm32samples/tree/master/F3:F303/NitrogenFlooding build#" BUILD_NUMBER " @ " BUILD_DATE "\n");
    while(c->cmd){
        if(!c->fn){ // header
            USB_sendstr("\n    ");
            USB_sendstr(c->cmd);
            USB_putbyte(':');
        }else{
            USB_sendstr(c->cmd);
            USB_sendstr(" - ");
            USB_sendstr(c->help);
        }
        newline();
        ++c;
    }
}

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
