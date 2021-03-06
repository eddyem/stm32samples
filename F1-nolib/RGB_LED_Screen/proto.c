/*
 * This file is part of the RGBLEDscreen project.
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

#include "adcrandom.h"
#include "balls.h"
#include "fonts.h"
#include "proto.h"
#include "screen.h"
#include "usb.h"

extern uint8_t countms, rainbow, balls;
extern uint32_t Tms;

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
    "'0/1' - screen off/on\n"
    "'2,3' - select font\n"
    "'A' - get ADC values\n"
    "'B' - start/stop rainBow\n"
    "'b' - start/stop Balls\n"
    "'C' - clear screen with given color\n"
    "'F' - set foreground color\n"
    "'G' - get 100 random numbers\n"
    "'f' - get FPS\n"
    "'R' - software reset\n"
    "'W' - test watchdog\n"
    "'Zz' -start/stop counting ms\n"
    "Any text - put text @ screen\n"
;

const char *parse_cmd(const char *buf){
    uint32_t N;
    if(buf[1] == '\n'){ // one symbol commands
        switch(*buf){
            case '0':
                ScreenOFF();
                return "OFF\n";
            break;
            case '1':
                ScreenON();
                return "ON\n";
            break;
            case '2':
                if(choose_font(FONT14)) return "Font14\n";
                return "err\n";
            break;
            case '3':
                if(choose_font(FONT16)) return "Font16\n";
                return "err\n";
            break;
            case 'A':
                USB_send("Tsens="); USB_send(u2str(getADCval(0)));
                USB_send("\nVref="); USB_send(u2str(getADCval(1)));
                USB_send("\nRand="); USB_send(u2str(getRand()));
                USB_send("\n");
                return NULL;
            break;
            case 'B':
                if(rainbow){
                    rainbow = 0;
                    return "Stop rainbow\n";
                }else{
                    rainbow = 1;
                    return "Start rainbow\n";
                }
            break;
            case 'b':
                if(balls){
                    balls = 0;
                    return "Stop balls\n";
                }else{
                    balls_init();
                    balls = 1;
                    return "Start balls\n";
                }
            case 'f':
                if(SCREEN_RELAX == getScreenState()) return "Screen is inactive\n";
                USB_send("FPS=");
                USB_send(u2str(getFPS()));
                USB_send("\n");
                return NULL;
            break;
            case 'G':
                /*USB_send(u2str(Tms)); USB_send("\n");
                for(int i=0; i < 1000; ++i) getRand();
                USB_send(u2str(Tms)); USB_send("\n");*/
                for(int i = 0; i < 100; ++i){
                    USB_send(u2str(getRand()));
                    USB_send("\n");
                }
                return NULL;
            break;
            case 'R':
                USB_send("Soft reset\n");
                NVIC_SystemReset();
            break;
            case 'W':
                USB_send("Wait for reboot\n");
                while(1){nop();};
            break;
            case 'Z':
                countms = 1;
                return "Start\n";
            break;
            case 'z':
                countms = 0;
                return "Stop\n";
            break;
            default:
                return helpmsg;
        }
        return NULL;
    }else{
        switch(*buf){
            case 'C':
                if(getnum(buf+1, &N)){
                    ScreenOFF();
                    setBGcolor(N);
                    ClearScreen();
                    ScreenON();
                    return "Background color\n";
                }
                return "Wrong color\n";
            break;
            case 'F':
                if(getnum(buf+1, &N)){
                    setFGcolor(N);
                    return "Foreground color\n";
                }
                return "Wrong color\n";
            break;
            default:
                ScreenOFF();
                ClearScreen();
                PutStringAt(1, curfont->height + 3, buf);
                ScreenON();
        }
        return buf;
    }
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
