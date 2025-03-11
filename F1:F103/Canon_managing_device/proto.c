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

#include <string.h>

#include "canon.h"
#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "spi.h"
#include "usb_dev.h"
#include "version.inc"

static const char *OK = "OK", *FAIL = "FAIL";

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
    "https://github.com/eddyem/stm32samples/tree/master/F1-nolib/Canon_managing_device  build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
    "0 - move to smallest foc value (e.g. 2.5m)\n"
    "1 - move to largest foc value (e.g. infinity)\n"
    "a - move focus to given ABSOLUTE position or get current value (without number)\n"
    "d - open/close diaphragm by 1 step (+/-), open/close fully (o/c) (no way to know it current status)\n"
    "f - move focus to given RELATIVE position\n"
    "h - turn on hand focus management\n"
    "i - get lens information\n"
    "l - get lens model\n"
    "r - get regulators' state\n"
    "\t\tdebugging/conf commands:\n"
    "A - set (!0) or reset (0) autoinit\n"
    "C - set CAN speed (25-3000 kbaud)\n"
    "D - set CAN ID (11 bit)\n"
    "E - erase full flash storage\n"
    "F - change SPI flags (F f val), f== l-LSBFIRST, b-BR [18MHz/2^(b+1)], p-CPOL, h-CPHA\n"
    "G - get SPI status\n"
    "I - reinit SPI\n"
    "L - 'flood' message (same as `S` but every 250ms until next command)\n"
    "P - dump current config\n"
    "R - software reset\n"
    "S - send data over SPI\n"
    "T - show Tms value\n"
    "X - save current config to flash\n"
;

#define SPIBUFSZ    (64)
// buffer for SPI sending
static uint8_t spibuf[SPIBUFSZ];
static int spibuflen; // length of spibuf
// put user data into buffer
static int initspibuf(const char *buf){
    uint32_t D;
    spibuflen = 0;
    do{
        const char *nxt = getnum(buf, &D);
        if(buf == nxt) break;
        buf = nxt;
        if(D > 0xff){
            USB_sendstr("Number should be from 0 to 0xff\n");
            return 0;
        }
        spibuf[spibuflen++] = (uint8_t)D;
    }while(spibuflen < SPIBUFSZ);
    return spibuflen;
}
static void sendspibuf(){
    if(spibuflen < 1) return;
    uint8_t buf[SPIBUFSZ];
    memcpy(buf, spibuf, spibuflen);
    if(spibuflen == SPI_transmit((uint8_t*)buf, (uint8_t)spibuflen)){
        USB_sendstr("Got SPI answer: ");
        for(int i = 0; i < spibuflen; ++i){
            if(i) USB_sendstr(", ");
            USB_sendstr(u2hexstr(buf[i]));
        }
        USB_sendstr("\n");
    }else USB_sendstr("Failed to send SPI buffer\n");
}

static void errw(int e){
    if(e){
        USB_sendstr("Error with code ");
        USB_sendstr(u2str(e));
        if(e == 1) USB_sendstr(" (busy or need initialization)");
    }else USB_sendstr(OK);
}

const char *connmsgs[LENS_S_AMOUNT+1] = {
    [LENS_DISCONNECTED] = "disconnected",
    [LENS_SLEEPING] = "sleeping, need init",
    [LENS_OVERCURRENT] = "overcurrent",
    [LENS_INITIALIZED] = "initialized",
    [LENS_READY] = "ready",
    [LENS_ERR] = "error",
    [LENS_S_AMOUNT] = "wrong state"
};
const char *inimsgs[INI_S_AMOUNT+1] = {
    [INI_START] = "started",
    [INI_FGOTOZ] = "go to min F",
    [INI_FPREPMAX] = "prepare to go to max F",
    [INI_FGOTOMAX] = "go to max F",
    [INI_FPREPOLD] = "prepare to return to original F",
    [INI_FGOTOOLD] = "go to starting F",
    [INI_READY] = "ready",
    [INI_ERR] = "error in init procedure",
    [INI_S_AMOUNT] = "wrong state"
};
void parse_cmd(const char *buf){
    static uint32_t lastFloodTime = 0;
    if(lastFloodTime && (Tms - lastFloodTime > FLOODING_INTERVAL)){
        sendspibuf();
        lastFloodTime = Tms ? Tms : 1;
    }
    if(!buf || *buf == 0) return;
    lastFloodTime= FALSE;
    if(buf[1] == '\n' || !buf[1]){ // one symbol commands
        switch(*buf){
            case 'a':
            case 'f':
                errw(canon_focus(-1));
            break;
            case '-':
                flashstorage_init();
            break;
            case '0':
                errw(canon_sendcmd(CANON_FMIN));
            break;
            case '1':
                errw(canon_sendcmd(CANON_FMAX));
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
            case 'E':
                if(erase_storage(-1)) USB_sendstr(FAIL);
                USB_sendstr(OK);
            break;
            case 'F': // just watch SPI->CR1 value
                USB_sendstr("SPI1->CR1="); USB_sendstr(u2hexstr(SPI_CR1));
            break;
            case 'G':
                USB_sendstr("SPI ");
                switch(SPI_status){
                    case SPI_NOTREADY:
                        USB_sendstr("not ready");
                    break;
                    case SPI_READY:
                        USB_sendstr("ready");
                    break;
                    case SPI_BUSY:
                        USB_sendstr("busy");
                    break;
                    default:
                        USB_sendstr("unknown");
                }
                USB_sendstr("\nstate=");
                uint16_t s = canon_getstate();
                uint8_t idx = s & 0xff;
                if(idx > LENS_S_AMOUNT) idx = LENS_S_AMOUNT;
                USB_sendstr(connmsgs[idx]);
                idx = s >> 8;
                USB_sendstr("\ninistate=");
                if(idx > INI_S_AMOUNT) idx = INI_S_AMOUNT;
                USB_sendstr(inimsgs[idx]);
            break;
            case 'h':
                errw(canon_sendcmd(CANON_FOCBYHANDS));
            break;
            case 'I':
                USB_sendstr("Reinit SPI\n");
                spi_setup();
                canon_init();
                return;
            break;
            case 'P':
                dump_userconf();
                return;
            break;
            case 'R':
                USB_sendstr("Soft reset\n");
                NVIC_SystemReset();
            break;
            case 'T':
                USB_sendstr("Tms=");
                USB_sendstr(u2str(Tms));
            break;
            case 'X':
                if(store_userconf()) USB_sendstr(FAIL);
                else USB_sendstr(OK);
            break;
            default:
                USB_sendstr(helpmsg);
        }
        newline();
        return;
    }
    uint32_t D = 0;
    int16_t neg;
    char *nxt;
    switch(*buf++){ // long messages
        case 'a': // move focus to absolute position
            buf = omit_spaces(buf);
            neg = 1;
            if(*buf == '-'){ ++buf; neg = -1; }
            nxt = getnum(buf, &D);
            if(nxt == buf) USB_sendstr("Need number");
            else if(D > 0x7fff) USB_sendstr("From -0x7fff to 0x7fff");
            else errw(canon_focus(neg * (int16_t)D));
        break;
        case 'A':
            nxt = getnum(buf, &D);
            if(nxt != buf){
                if(D) the_conf.autoinit = 1;
                else the_conf.autoinit = 0;
            }
            USB_sendstr("autoinit="); USB_sendstr(u2str(the_conf.autoinit)); USB_sendstr("\n");
        break;
        case 'd':
            nxt = omit_spaces(buf);
            errw(canon_diaphragm(*nxt));
        break;
        case 'f': // move focus to relative position
            buf = omit_spaces(buf);
            neg = 1;
            if(*buf == '-'){ ++buf; neg = -1; }
            nxt = getnum(buf, &D);
            if(nxt == buf) USB_sendstr("Need number");
            else if(D > 0x7fff) USB_sendstr("From -0x7fff to 0x7fff");
            else{
                if(canon_writeu16(CANON_FOCMOVE, neg * (int16_t)D)) USB_sendstr(OK);
                else USB_sendstr(FAIL);
            }
        break;
        case 'C':
            nxt = getnum(buf, &D);
            if(nxt != buf && D >= 25 && D <= 3000){
                the_conf.canspeed = D;
                USB_sendstr("CAN_speed="); USB_sendstr(u2str(the_conf.canspeed));
            }else USB_sendstr(FAIL);
        break;
        case 'D':
            nxt = getnum(buf, &D);
            if(nxt != buf && D < 0x800){
                the_conf.canID = D;
                USB_sendstr("CAN_ID="); USB_sendstr(u2str(the_conf.canID));
            }else USB_sendstr(FAIL);
        break;
        case 'F': // SPI flags
            nxt = omit_spaces(buf);
            char c = *nxt;
            if(*nxt && *nxt != '\n'){
                buf = nxt + 1;
                nxt = getnum(buf, &D);
                if(buf == nxt || D > 7){
                    USB_sendstr(helpmsg);
                    return;
                }
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
                    USB_sendstr(helpmsg);
                    return;
            }
            USB_sendstr("SPI_CR1="); USB_sendstr(u2hexstr(SPI_CR1));
        break;
        case 'L':
            if(0 == initspibuf(buf)){
                USB_sendstr("Enter data bytes\n");
                return;
            }
            USB_sendstr("OK, activated\n");
            sendspibuf();
            lastFloodTime = Tms ? Tms : 1;
            return;
        break;
        case 'S': // use stbuf here to store user data
            if(0 == initspibuf(buf)){
                USB_sendstr("Enter data bytes\n");
                return;
            }
            USB_sendstr("Send: ");
            for(int i = 0; i < spibuflen; ++i){
                if(i) USB_sendstr(", ");
                USB_sendstr(u2hexstr(spibuf[i]));
            }
            USB_sendstr("\n");
            sendspibuf();
            return;
        break;
        default:
            return;
    }
    newline();
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
