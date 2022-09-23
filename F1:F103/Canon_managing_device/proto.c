/*
 * This file is part of the canonmanage project.
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

#include "canon.h"
#include "hardware.h"
#include "proto.h"
#include "spi.h"
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
    "https://github.com/eddyem/stm32samples/tree/master/F1-nolib/USB_Canon_management build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
    "0 - move to smallest foc value (e.g. 2.5m)\n"
    "1 - move to largest foc value (e.g. infinity)\n"
    "d - open/close diaphragm by 1 step (+/-), open/close fully (o/c) (no way to know it current status)\n"
    "f - get focus state or move it to given relative position\n"
    "h - turn on hand focus management\n"
    "i - get lens information\n"
    "l - get lens model\n"
    "r - get regulators' state\n"
    "\t\tdebugging commands:\n"
    "F - change SPI flags (F f val), f== l-LSBFIRST, b-BR [18MHz/2^(b+1)], p-CPOL, h-CPHA\n"
    "G - get SPI status\n"
    "I - reinit SPI\n"
    "R - software reset\n"
    "S - send data over SPI\n"
    "T - show Tms value\n"
;

#define STBUFSZ 255
static char stbuf[STBUFSZ+1], *bptr = NULL;
static int blen = 0;
static void initbuf(){bptr = stbuf; blen = STBUFSZ; *bptr = 0;}
#define newline()   do{if(blen){ *bptr++ = '\n'; *bptr = 0; --blen; }}while(0)
static void add2buf(const char *s){
    while(blen && *s){
        *bptr++ = *s++;
        --blen;
    }
    *bptr = 0;
}

static void errw(int e){
    if(e){
        add2buf("Error with code ");
        add2buf(u2str(e));
        if(e == 1) add2buf(" (busy or need initialization)");
    }else add2buf("OK");
}

extern uint8_t usbON;
const char *parse_cmd(const char *buf){
    //uint32_t u3;
    initbuf();
    if(buf[1] == '\n' || !buf[1]){ // one symbol commands
        switch(*buf){
            case '0':
                errw(canon_sendcmd(CANON_FMIN));
            break;
            case '1':
                errw(canon_sendcmd(CANON_FMAX));
            break;
            case 'f':
                errw(canon_focus(0));
            break;
            case 'i':
                errw(canon_getinfo());
            break;
            case 'l':
                errw(canon_asku16(CANON_GETMODEL));
            break;
            case 'r':
                errw(canon_asku16(CANON_GETREG));
            break;
            case 'F': // just watch SPI->CR1 value
                add2buf("SPI1->CR1="); add2buf(u2hexstr(SPI_CR1));
            break;
            case 'G':
                add2buf("SPI ");
                switch(SPI_status){
                    case SPI_NOTREADY:
                        add2buf("not ready");
                    break;
                    case SPI_READY:
                        add2buf("ready");
                    break;
                    case SPI_BUSY:
                        add2buf("busy");
                    break;
                    default:
                        add2buf("unknown");
                }
            break;
            case 'h':
                errw(canon_sendcmd(CANON_FOCBYHANDS));
            break;
            case 'I':
                add2buf("Reinit SPI");
                spi_setup();
                canon_init();
            break;
            case 'R':
                USB_send("Soft reset\n");
                NVIC_SystemReset();
            break;
            case 'T':
                add2buf("Tms=");
                add2buf(u2str(Tms));
            break;
            default:
                return helpmsg;
        }
        newline();
        return stbuf;
    }
    uint32_t D = 0, N = 0;
    char *nxt;
    switch(*buf){ // long messages
        case 'd':
            nxt = omit_spaces(buf+1);
            errw(canon_diaphragm(*nxt));
        break;
        case 'f': // move focus
            buf = omit_spaces(buf + 1);
            int16_t neg = 1;
            if(*buf == '-'){ ++buf; neg = -1; }
            nxt = getnum(buf, &D);
            if(nxt == buf) add2buf("Need number");
            else if(D > 0x7fff) add2buf("From -0x7fff to 0x7fff");
            else errw(canon_focus(neg * (int32_t)D));
        break;
        case 'F': // SPI flags
            nxt = omit_spaces(buf+1);
            char c = *nxt;
            if(*nxt && *nxt != '\n'){
                buf = nxt + 1;
                nxt = getnum(buf, &D);
                if(buf == nxt || D > 7) return helpmsg;
            }
            switch(c){
                case 'b':
                    SPI_CR1 &= ~SPI_CR1_BR;
                    SPI_CR1 |= ((uint8_t)D) << 3;
                break;
                case 'h':
                    if(D) SPI_CR1 |= SPI_CR1_CPHA;
                    else SPI_CR1 &= ~SPI_CR1_CPHA;
                break;
                case 'l':
                    if(D) SPI_CR1 |= SPI_CR1_LSBFIRST;
                    else SPI_CR1 &= ~SPI_CR1_LSBFIRST;
                break;
                case 'p':
                    if(D) SPI_CR1 |= SPI_CR1_CPOL;
                    else SPI_CR1 &= ~SPI_CR1_CPOL;
                break;
                default:
                    return helpmsg;
            }
            add2buf("SPI_CR1="); add2buf(u2hexstr(SPI_CR1));
        break;
        case 'S': // use stbuf here to store user data
            ++buf;
            do{
                nxt = getnum(buf, &D);
                if(buf == nxt) break;
                buf = nxt;
                if(D > 0xff){
                    USB_send("Number should be from 0 to 0xff\n");
                    return NULL;
                }
                stbuf[N++] = (uint8_t)D;
                if(N == STBUFSZ) break;
            }while(1);
            if(N == 0){
                USB_send("Enter data bytes\n");
                return NULL;
            }
            USB_send("Send: ");
            for(uint32_t i = 0; i < N; ++i){
                if(i) USB_send(", ");
                USB_send(u2hexstr(stbuf[i]));
            }
            USB_send("\n... ");
            canon_setlastcmd(stbuf[0]);
            if(N == SPI_transmit((uint8_t*)stbuf, (uint8_t)N)) USB_send("OK\n");
            else USB_send("Failed\n");
            return NULL;
        break;
        default:
            return buf;
    }
    newline();
    return stbuf;
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
