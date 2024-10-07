/*
 * This file is part of the shutter project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "shutter.h"
#include "usb.h"
#include "version.inc"

char *omit_spaces(const char *buf){
    while(*buf){
        if(*buf > ' ') break;
        ++buf;
    }
    return (char*)buf;
}

// In case of overflow return `buf` and N==0xffffffff
// read decimal number & return pointer to next non-number symbol
static char *getdec(const char *buf, uint32_t *N){
    char *start = (char*)buf;
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '9'){
            break;
        }
        if(num > 429496729 || (num == 429496729 && c > '5')){ // overflow
            *N = 0xffffff;
            return start;
        }
        num *= 10;
        num += c - '0';
        ++buf;
    }
    *N = num;
    return (char*)buf;
}
// read hexadecimal number (without 0x prefix!)
static char *gethex(const char *buf, uint32_t *N){
    char *start = (char*)buf;
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        uint8_t M = 0;
        if(c >= '0' && c <= '9'){
            M = '0';
        }else if(c >= 'A' && c <= 'F'){
            M = 'A' - 10;
        }else if(c >= 'a' && c <= 'f'){
            M = 'a' - 10;
        }
        if(M){
            if(num & 0xf0000000){ // overflow
                *N = 0xffffff;
                return start;
            }
            num <<= 4;
            num += c - M;
        }else{
            break;
        }
        ++buf;
    }
    *N = num;
    return (char*)buf;
}
// read octal number (without 0 prefix!)
static char *getoct(const char *buf, uint32_t *N){
    char *start = (char*)buf;
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '7'){
            break;
        }
        if(num & 0xe0000000){ // overflow
            *N = 0xffffff;
            return start;
        }
        num <<= 3;
        num += c - '0';
        ++buf;
    }
    *N = num;
    return (char*)buf;
}
// read binary number (without b prefix!)
static char *getbin(const char *buf, uint32_t *N){
    char *start = (char*)buf;
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '1'){
            break;
        }
        if(num & 0x80000000){ // overflow
            *N = 0xffffff;
            return start;
        }
        num <<= 1;
        if(c == '1') num |= 1;
        ++buf;
    }
    *N = num;
    return (char*)buf;
}

/**
 * @brief getnum - read uint32_t from string (dec, hex or bin: 127, 0x7f, 0b1111111)
 * @param buf - buffer with number and so on
 * @param N   - the number read
 * @return pointer to first non-number symbol in buf
 *      (if it is == buf, there's no number or if *N==0xffffffff there was overflow)
 */
char *getnum(const char *txt, uint32_t *N){
    char *nxt = NULL;
    char *s = omit_spaces(txt);
    if(*s == '0'){ // hex, oct or 0
        if(s[1] == 'x' || s[1] == 'X'){ // hex
            nxt = gethex(s+2, N);
            if(nxt == s+2) nxt = (char*)txt;
        }else if(s[1] > '0'-1 && s[1] < '8'){ // oct
            nxt = getoct(s+1, N);
            if(nxt == s+1) nxt = (char*)txt;
        }else{ // 0
            nxt = s+1;
            *N = 0;
        }
    }else if(*s == 'b' || *s == 'B'){
        nxt = getbin(s+1, N);
        if(nxt == s+1) nxt = (char*)txt;
    }else{
        nxt = getdec(s, N);
        if(nxt == s) nxt = (char*)txt;
    }
    return nxt;
}

const char* helpmsg =
    "https://github.com/eddyem/stm32samples/tree/master/F1:F103/shutter build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
    "    common control:\n"
    "'A' - get raw ADC values\n"
    "'C' - close shutter / abort exposition\n"
    "'E n' - expose for n milliseconds\n"
    "'O' - open shutter\n"
    "'P n' - set shutter voltage PWM to n (0..100)\n"
    "'R' - software reset\n"
    "'S' - get shutter state\n"
    "'t' - get MCU temperature (/10degC)\n"
    "'T' - get Tms\n"
    "'v' - get Vdd (/100V)\n"
    "'V' - get shutter voltage (/100V)\n"
    "    configuration:\n"
    "'> n' - minimal working voltage (*100)\n"
    "'# n' - duration of electrical pulse to open/close shutter (ms)\n"
    "'* n' - shutter voltage multiplier\n"
    "'/ n' - shutter voltage divider (V=Vadc*M/D)\n"
    "'c n' - open shutter when CCD ext level is n (0/1)\n"
    "'d' - dump current config\n"
    "'e' - erase flash storage\n"
    "'h n' - shutter is opened when sensor level is n (0/1)\n"
    "'k n' - check (1) or not (0) shutter state sensor\n"
    "'p n' - set holding PWM level to n (0..100%)\n"
    "'s' - save configuration into flash\n"
;

static const char *OK = "OK", *ERR = "ERR", *ERRVAL = "ERRVAL\n";
const char *parse_cmd(const char *buf){
    if(buf[1] == '\n' || buf[1] == '\r' || !buf[1]){ // one symbol commands
        switch(*buf){
            case 'd': // dump config
                dump_userconf(); return NULL;
            break;
            case 'e': // erase flash
                if(erase_storage(-1)) USB_sendstr(ERR);
                else USB_sendstr(OK);
            break;
            case 's': // save configuration
                if(store_userconf()) USB_sendstr(ERR);
                else USB_sendstr(OK);
            break;
            case 'A':
                for(int i = 0; i < NUMBER_OF_ADC_CHANNELS; ++i){
                    USB_sendstr("\nadc");
                    USB_putbyte('0' + i);
                    USB_putbyte('=');
                    USB_sendstr(u2str(getADCval(i)));
                }
            break;
            case 'C':
                if(close_shutter()) USB_sendstr(OK);
                else USB_sendstr(ERR);
            break;
            case 'O':
                if(open_shutter()) USB_sendstr(OK);
                else USB_sendstr(ERR);
            break;
            case 'R':
                USB_sendstr("Soft reset\n");
                USB_sendall();
                NVIC_SystemReset();
            break;
            case 'S':
                print_shutter_state();
                USB_sendstr("\nccd=");
                USB_putbyte('0' + CHKCCD());
            break;
            break;
            case 't':
                USB_sendstr("mcut=");
                USB_sendstr(u2str(getMCUtemp()));
            break;
            case 'T':
                USB_sendstr("tms=");
                USB_sendstr(u2str(Tms));
            break;
            case 'v':
                USB_sendstr("vdd=");
                USB_sendstr(u2str(getVdd()));
            break;
            case 'V':
                USB_sendstr("voltage=");
                USB_sendstr(u2str(getShutterVoltage()));
            break;
            default:
                return helpmsg;
        }
    }else{ // long messages: cmd + number
        uint32_t Num = 0;
        char c = *buf++;
        char *nxt = getnum(buf, &Num);
        if(buf == nxt){
            if(Num) return "I32OVERFLOW\n";
            return "ERRNUM\n";
        }
        switch(c){
            case '>': // max voltage
                if(Num < 500 || Num > 10000) return ERRVAL;
                the_conf.workvoltage = Num;
                USB_sendstr("workvoltage="); USB_sendstr(u2str(the_conf.workvoltage));
            break;
            case '#': // shuttertime
                if(Num < 5 || Num > 60000) return ERRVAL;
                the_conf.shuttertime = Num;
                USB_sendstr("shuttertime="); USB_sendstr(u2str(the_conf.shuttertime));
            break;
            case '*': // mult
                if(Num < 1 || Num > 65535) return ERRVAL; // avoid zeroing and overfull
                the_conf.shtrVmul = Num;
                USB_sendstr("shtrvmul="); USB_sendstr(u2str(the_conf.shtrVmul));
            break;
            case '/': // div
                if(Num < 1 || Num > 65535) return ERRVAL; // avoid zero dividing and overfull
                the_conf.shtrVdiv = Num;
                USB_sendstr("shtrvdiv="); USB_sendstr(u2str(the_conf.shtrVdiv));
            break;
            case 'c': // CCD active @
                the_conf.ccdactive = Num;
                USB_sendstr("ccdactive="); USB_putbyte('0' + the_conf.ccdactive);
            break;
            case 'h': // HALL active @
                the_conf.hallactive = Num;
                USB_sendstr("hallactive="); USB_putbyte('0' + the_conf.hallactive);
            break;
            case 'k': // check shutter state
                the_conf.chkhall = Num;
                USB_sendstr("chkhall="); USB_putbyte('0' + the_conf.chkhall);
            break;
            case 'P':
                if(Num > 100) return ERRVAL;
                set_pwm(Num);
                USB_sendstr(OK);
            break;
            case 'p': // set holding PWM
                if(Num > 100) return ERRVAL;
                the_conf.holdingPWM = Num;
                USB_sendstr("holdpwm="); USB_sendstr(u2str(the_conf.holdingPWM));
            break;
            case 'E':
                if(shutterstate != SHUTTER_CLOSED){
                    USB_sendstr(ERR);
                    break;
                }
                if(expose_shutter(Num)) USB_sendstr(OK);
                else USB_sendstr(ERR);
            break;
            default:
                return "Wrongcmd\n";
        }
    }
    USB_putbyte('\n');
    return NULL;
}


// return string with number `val`
char *u2str(uint32_t val){
    static char strbuf[11];
    char *bufptr = &strbuf[10];
    *bufptr = 0;
    if(!val){
        *(--bufptr) = '0';
    }else{
        while(val){
            *(--bufptr) = val % 10 + '0';
            val /= 10;
        }
    }
    return bufptr;
}

char *u2hexstr(uint32_t val){
    static char strbuf[11] = "0x";
    char *sptr = strbuf + 2;
    uint8_t *ptr = (uint8_t*)&val + 3;
    int8_t i, j, z=1;
    for(i = 0; i < 4; ++i, --ptr){
        if(*ptr == 0){ // omit leading zeros
            if(i == 3) z = 0;
            if(z) continue;
        }
        else z = 0;
        for(j = 1; j > -1; --j){
            uint8_t half = (*ptr >> (4*j)) & 0x0f;
            if(half < 10) *sptr++ = half + '0';
            else *sptr++ = half - 10 + 'a';
        }
    }
    *sptr = 0;
    return strbuf;
}
