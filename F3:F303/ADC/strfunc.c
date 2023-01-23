/*
 * This file is part of the adc project.
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

#include <math.h>
#include <stm32f3.h>
#include <string.h>
/**
 * @brief hexdump - dump hex array by 16 bytes in string
 * @param sendfun - function to send data
 * @param arr - array to dump
 * @param len - length of `arr`
 */
void hexdump(int (*sendfun)(const char *s), uint8_t *arr, uint16_t len){
    char buf[52], *bptr = buf;
    for(uint16_t l = 0; l < len; ++l, ++arr){
        for(int16_t j = 1; j > -1; --j){
            register uint8_t half = (*arr >> (4*j)) & 0x0f;
            if(half < 10) *bptr++ = half + '0';
            else *bptr++ = half - 10 + 'a';
        }
        if(l % 16 == 15){
            *bptr++ = '\n';
            *bptr = 0;
            sendfun(buf);
            bptr = buf;
        }else *bptr++ = ' ';
    }
    if(bptr != buf){
        *bptr++ = '\n';
        *bptr = 0;
        sendfun(buf);
    }
}

/**
 * @brief _2str - convert value into string buffer
 * @param val - |value|
 * @param minus - ==0 if value > 0
 * @return buffer with number
 */
static char *_2str(uint32_t  val, uint8_t minus){
    static char strbuf[12];
    char *bufptr = &strbuf[11];
    *bufptr = 0;
    if(!val){
        *(--bufptr) = '0';
    }else{
        while(val){
            uint32_t x = val / 10;
            *(--bufptr) = (val - 10*x) + '0';
            val = x;
            //*(--bufptr) = val % 10 + '0';
            //val /= 10;
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

/**
 * @brief uhex2str - print 32bit unsigned int as hex
 * @param val - value
 * @return string with number
 */
char *uhex2str(uint32_t val){
    static char buf[12] = "0x";
    int npos = 2;
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
            if(half < 10) buf[npos++] = half + '0';
            else buf[npos++] = half - 10 + 'a';
        }
    }
    buf[npos] = 0;
    return buf;
}

/**
 * @brief omit_spaces - eliminate leading spaces and other trash in string
 * @param buf - string
 * @return - pointer to first character in `buf` > ' '
 */
const char *omit_spaces(const char *buf){
    while(*buf){
        if(*buf > ' ') break;
        ++buf;
    }
    return buf;
}

/**
 * @brief getdec - read decimal number & return pointer to next non-number symbol
 * @param buf - string
 * @param N - number read
 * @return Next non-number symbol. In case of overflow return `buf` and N==0xffffffff
 */
static const char *getdec(const char *buf, uint32_t *N){
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
    return buf;
}
// read hexadecimal number (without 0x prefix!)
static const char *gethex(const char *buf, uint32_t *N){
    const char *start = buf;
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
    return buf;
}
// read octal number (without 0 prefix!)
static const char *getoct(const char *buf, uint32_t *N){
    const char *start = (char*)buf;
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
    return buf;
}
// read binary number (without b prefix!)
static const char *getbin(const char *buf, uint32_t *N){
    const char *start = (char*)buf;
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
    return buf;
}

/**
 * @brief getnum - read uint32_t from string (dec, hex or bin: 127, 0x7f, 0b1111111)
 * @param buf - buffer with number and so on
 * @param N   - the number read
 * @return pointer to first non-number symbol in buf
 *      (if it is == buf, there's no number or if *N==0xffffffff there was overflow)
 */
const char *getnum(const char *txt, uint32_t *N){
    const char *nxt = NULL;
    const char *s = omit_spaces(txt);
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

// get signed integer
const char *getint(const char *txt, int32_t *I){
    const char *s = omit_spaces(txt);
    int32_t sign = 1;
    uint32_t U;
    if(*s == '-'){
    	sign = -1;
    	++s;
    }
    const char *nxt = getnum(s, &U);
    if(nxt == s) return txt;
    if(U & 0x80000000) return txt; // overfull
    *I = sign * (int32_t)U;
    return nxt;
}

/*
void mymemcpy(char *dest, const char *src, int len){
    if(len < 1) return;
    while(len--) *dest++ = *src++;
}
*/

// be careful: if pow10 would be bigger you should change str[] size!
static const float pwr10[] = {1.f, 10.f, 100.f, 1000.f, 10000.f};
static const float rounds[] = {0.5f, 0.05f, 0.005f, 0.0005f, 0.00005f};
#define P10L  (sizeof(pwr10)/sizeof(uint32_t) - 1)
char * float2str(float x, uint8_t prec){
    static char str[16] = {0}; // -117.5494E-36\0 - 14 symbols max!
    if(prec > P10L) prec = P10L;
    if(isnan(x)){ memcpy(str, "NAN", 4); return str;}
    else{
        int i = isinf(x);
        if(i){memcpy(str, "-INF", 5); if(i == 1) return str+1; else return str;}
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
        units = (uint32_t) (x + 0.5);
    }
    // print main units
    if(units == 0) *s-- = '0';
    else while(units){
        register uint32_t u10 = units / 10;
        *s-- = '0' + (units - 10*u10);
        units = u10;
    }
    if(minus) *s-- = '-';
    return s+1;
}
