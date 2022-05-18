/*
 * This file is part of the MLX90640 project.
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

#include "hardware.h"
#include "strfunct.h"
#include "usb.h"

#include <string.h> // strlen

// usb getline
char *get_USB(){
    static char tmpbuf[IBUFSZ+1], *curptr = tmpbuf;
    static int rest = IBUFSZ;
    uint8_t x = USB_receive((uint8_t*)curptr);
    curptr[x] = 0;
    if(!x) return NULL;
    if(curptr[x-1] == '\n'){
        curptr[x] = 0;
        curptr = tmpbuf;
        rest = IBUFSZ;
        return tmpbuf;
    }
    curptr += x; rest -= x;
    if(rest <= 63){ // buffer overflow
        curptr = tmpbuf;
        rest = IBUFSZ;
    }
    return NULL;
}

static char buff[OBUFSZ+1], *bptr = buff;
static uint16_t blen = 0;

void sendbuf(){
    if(blen == 0) return;
    *bptr = 0;
    USB_sendstr(buff);
    bptr = buff;
    blen = 0;
}

void bufputchar(char ch){
    if(blen > OBUFSZ-1){
        sendbuf();
    }
    *bptr++ = ch;
    ++blen;
}

void addtobuf(const char *txt){
    if(!txt) return;
    while(*txt) bufputchar(*txt++);
}

// print 32bit unsigned int
void printu(uint32_t val){
    char buf[11], *bufptr = &buf[10];
    *bufptr = 0;
    if(!val){
        *(--bufptr) = '0';
    }else{
        while(val){
            register uint32_t o = val;
            val /= 10;
            *(--bufptr) = (o - 10*val) + '0';
        }
    }
    addtobuf(bufptr);
}
void printi(int32_t val){
    if(val < 0){
        val = -val;
        bufputchar('-');
    }
    printu((uint32_t)val);
}

// print 32bit unsigned int as hex
void printuhex(uint32_t val){
    addtobuf("0x");
    uint8_t *ptr = (uint8_t*)&val + 3;
    int i, j, z = 1;
    for(i = 0; i < 4; ++i, --ptr){
        if(*ptr == 0){ // omit leading zeros
            if(i == 3) z = 0;
            if(z) continue;
        }
        else z = 0;
        for(j = 1; j > -1; --j){
            uint8_t half = (*ptr >> (4*j)) & 0x0f;
            if(half < 10) bufputchar(half + '0');
            else bufputchar(half - 10 + 'a');
        }
    }
}

char *omit_spaces(char *buf){
    while(*buf){
        if(*buf != ' ' && *buf != '\t') break;
        ++buf;
    }
    return buf;
}

// THERE'S NO OVERFLOW PROTECTION IN NUMBER READ PROCEDURES!
// read decimal number
static char *getdec(const char *buf, int32_t *N){
    int32_t num = 0;
    int positive = TRUE;
    if(*buf == '-'){
        positive = FALSE;
        ++buf;
    }
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '9'){
            break;
        }
        num *= 10;
        num += c - '0';
        ++buf;
    }
    *N = (positive) ? num : -num;
    return (char *)buf;
}
// read hexadecimal number (without 0x prefix!)
static char *gethex(const char *buf, int32_t *N){
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
            num <<= 4;
            num += c - M;
        }else{
            break;
        }
        ++buf;
    }
    *N = (int32_t)num;
    return (char *)buf;
}
// read binary number (without 0b prefix!)
static char *getbin(const char *buf, int32_t *N){
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '1'){
            break;
        }
        num <<= 1;
        if(c == '1') num |= 1;
        ++buf;
    }
    *N = (int32_t)num;
    return (char *)buf;
}

/**
 * @brief getnum - read uint32_t from string (dec, hex or bin: 127, 0x7f, 0b1111111)
 * @param buf - buffer with number and so on
 * @param N   - the number read
 * @return pointer to first non-number symbol in buf (if it is == buf, there's no number)
 */
char *getnum(char *txt, int32_t *N){
    txt = omit_spaces(txt);
    if(*txt == '0'){
        if(txt[1] == 'x' || txt[1] == 'X') return gethex(txt+2, N);
        if(txt[1] == 'b' || txt[1] == 'B') return getbin(txt+2, N);
    }
    return getdec(txt, N);
}

// be careful: if pow10 would be bigger you should change str[] size!
static const float pwr10[] = {1., 10., 100., 1000., 10000.};
static const float rounds[] = {0.5, 0.05, 0.005, 0.0005, 0.00005};
#define P10L  (sizeof(pwr10)/sizeof(uint32_t) - 1)
void float2str(float x, uint8_t prec){
    if(prec > P10L) prec = P10L;
    static char str[16] = {0}; // -117.5494E-36\0 - 14 symbols max!
    uint32_t *u = (uint32_t*)&x;
   /* if(*u && (*u == (*u & DENORM))){
        SEND("DENORM"); return;
    }*/
    switch(*u){
        case INF:
            SEND("INF");
            return;
        break;
        case MINF:
            SEND("-INF");
            return;
        break;
        case NAN:
            SEND("NAN");
            return;
        default:
        break;
    }
    char *s = str + 14; // go to end of buffer
    uint8_t minus = 0;
    if(x < 0){
        x = -x;
        minus = 1;
    }
    int pow = 0; // xxxEpow
    // now convert float to 1.xxxE3y
    while(x > 1000.f){
        x /= 1000.f;
        pow += 3;
    }
    if(x > 0.) while(x < 1.){
        x *= 1000.f;
        pow -= 3;
    }
    // print Eyy
    if(pow){
        uint8_t m = 0;
        if(pow < 0){pow = -pow; m = 1;}
        while(pow){
            register int p10 = pow/10;
            *s-- = '0' + (pow - 10*p10);
            pow = p10;
        }
        if(m) *s-- = '-';
        *s-- = 'E';
    }
    // now our number is in [1, 1000]
    uint32_t units;
    if(prec){
        units = (uint32_t) x;
        uint32_t decimals = (uint32_t)((x-units+rounds[prec])*pwr10[prec]);
        // print decimals
        while(prec){
            register int d10 = decimals / 10;
            *s-- = '0' + (decimals - 10*d10);
            decimals = d10;
            --prec;
        }
        // decimal point
        *s-- = '.';
    }else{ // without decimal part
        units = (uint32_t) (x + 0.5f);
    }
    // print main units
    if(units == 0) *s-- = '0';
    else while(units){
        register uint32_t u10 = units / 10;
        *s-- = '0' + (units - 10*u10);
        units = u10;
    }
    if(minus) *s-- = '-';
    addtobuf(s+1);
}
