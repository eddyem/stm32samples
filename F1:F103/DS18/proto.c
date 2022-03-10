/*
 * This file is part of the DS18 project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "ds18.h"
#include "proto.h"
#include "usb.h"

extern volatile uint32_t Tms;


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

static const char *helpmesg =
        "'C' - clear match ROM\n"
        "'D' - get DS18 state\n"
        "'I' - get DS18 ID\n"
        "'M' - start measurement\n"
        "'P' - read scratchpad\n"
        "'S' - set match ROM (S 0xaa 0xbb... - 8 bytes of ID)\n"
        "'R' - software reset\n"
        "'T' - get time from start\n"
        "'W' - test watchdog\n"
;

// read 8 bytes of ROM and run SETROM
static void setROM(const char *buf){
    uint8_t ID[8];
    uint32_t N;
    for(int i = 0; i < 8; ++i){
        char *nxt = getnum(buf, &N);
        if(nxt == buf || N > 0xff){
            USB_send("Wrong data: ");
            USB_send(buf);
            return;
        }
        ID[i] = (uint8_t) N;
        buf = nxt;
    }
    DS18_setID(ID);
    USB_send("Set ID to:");
    for(int i = 0; i < 8; ++i){
        USB_send(" 0x");
        printhex(ID[i]);
    }
    USB_send("\n");
}

const char *parse_cmd(const char *buf){
    if(buf[1] == '\n'){
        switch(*buf){
            case 'C':
                DS18_clearID();
                return "Don't send MATCH ROM\n";
            break;
            case 'D':
                switch(DS18_getstate()){
                    case DS18_SLEEP:
                        return "Sleeping\n";
                    case DS18_RESETTING:
                        return "Resetting\n";
                    case DS18_DETECTING:
                        return "Detecting\n";
                    case DS18_DETDONE:
                        return "Detection done\n";
                    case DS18_READING:
                        return "Measuring\n";
                    case DS18_GOTANSWER:
                        return "Results are ready\n";
                    default:
                        return "Not found\n";
                }
            break;
            case 'I':
                if(DS18_readID()) return "Reading ID..\n";
                else return "Error\n";
            case 'M':
                if(DS18_start()) return "Started\n";
                else return "Wait a little\n";
            break;
    /*        case 'p':
                DS18_poll();
                return "polling\n";
            break;*/
            case 'P':
                if(DS18_readScratchpad()) return "Reading scr..\n";
                else return "Error\n";
            break;
            case 'R':
                USB_send("Soft reset\n");
                NVIC_SystemReset();
            break;
            case 'T':
                USB_send("T=");
                USB_send(u2str(Tms));
                USB_send("ms\n");
            break;
            case 'W':
                USB_send("Wait for reboot\n");
                while(1){nop();};
            break;
            default: // help
                return helpmesg;
            break;
        }
    }else{ // several letters
        switch(*buf){
            case 'S':
                setROM(buf + 1);
            break;
            default:
                return buf;
        }
    }
    return NULL;
}

static char *_2str(uint32_t  val, uint8_t minus){
    static char strbuf[12];
    char *bufptr = &strbuf[11];
    *bufptr = 0;
    if(!val){
        *(--bufptr) = '0';
    }else{
        while(val){
            *(--bufptr) = val % 10 + '0';
            val /= 10;
        }
    }
    if(minus) *(--bufptr) = '-';
    return bufptr;
}

// return string with number `val`
char *u2str(uint32_t val){
    return _2str(val, 0);
}
char *i2str(int32_t i){
    uint8_t minus = 0;
    uint32_t val;
    if(i < 0){
        minus = 1;
        val = -i;
    }else val = i;
    return _2str(val, minus);
}

void printhex(uint8_t x){
    char b[3];
    uint8_t H(uint8_t c){
        if(c < 10)
            return (c + '0');
        else
            return (c + 'a' - 10);
    }
    b[0] = H(x >> 4);
    b[1] = H(x & 0x0f);
    b[2] = 0;
    USB_send(b);
}

void printT(int16_t T){ // print temperature value, this function should be called from ds18.c
    USB_send("Temperature="); USB_send(i2str(T));
    USB_send("\n");
}

void printsp(uint8_t *b, int n){
    USB_send("Regvalue:");
    for(int i = 0; i < n; ++i){
        USB_send(" 0x");
        printhex(b[i]);
    }
    USB_send("\n");
}
