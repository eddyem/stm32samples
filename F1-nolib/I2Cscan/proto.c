/*
 * This file is part of the I2Cscan project.
 * Copyright 2020 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "i2cscan.h"
#include "i2c.h"
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

static void displaydata(uint8_t *data, int N){
    if(N == 0){
        USB_send("Done\n");
        return;
    }
    USB_send("Got: ");
    for(int i = 0; i < N; ++i){
        USB_send(u2str(data[i]));
        USB_send(" ");
    }
    USB_send("\n");
}

const char* helpmsg =
    "'Aa'- set I2C 7bit address a\n"
    "'I' - reinit i2c\n"
    "'Rrn'- read Nnbytes (<256) from register r\n"
    "'S' - scan I2C bus\n"
    "'Wrb'- write b to register r\n"
;

const char *parse_cmd(const char *buf){
    if(buf[1] == '\n'){ // one symbol commands
        switch(*buf){
            case 'I':
                i2c_setup();
                return "Reinit\n";
            break;
            case 'S':
                init_scan_mode();
                return "Scan mode\n";
            break;
            default:
                return helpmsg;
        }
        return NULL;
    }
    uint32_t Num = 0;
    uint8_t n, data[128];
    char *bufori = (char*)buf, *ptr;
    switch(*buf){ // long messages
        case 'A':
            ++buf;
            if(buf != getnum(buf, &Num)){
                if(Num & 0x80) return "Enter 7bit address\n";
                i2c_set_addr7(Num);
                return "Changed\n";
            }else return "Wrong filter number";
        break;
        case 'R':
            ++buf;
            if(buf != (ptr = getnum(buf, &Num))){
                n = Num;
                if(ptr != getnum(ptr, &Num)){
                    if(Num > 128) return "Not more than 128 bytes\n";
                    if(read_reg(n, data, Num)) displaydata(data, Num);
                    else return "Error\n";
                }else return "Need data amount to read\n";
            }else return "Need register address\n";
        break;
        case 'W':
            ++buf;
            if(buf != (ptr = getnum(buf, &Num))){
                n = Num;
                if(ptr != getnum(ptr, &Num)){
                    if(write_reg(n, Num)) return "Sent\n";
                    else return "Error\n";
                }else return "Need data to write\n";
            }else return "Need register address\n";
        break;
        default:
            return bufori;
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
