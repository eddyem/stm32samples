/*
 * This file is part of the si7005 project.
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

#include "dewpoint.h"
#include "proto.h"
#include "si7005.h"
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

const char* helpmsg =
    "'0' - reset I2C\n"
    "'I' - read SI7005 device ID\n"
    "'H' - start Humidity measurement\n"
    "'N' - read number (0..1000) and show its ln\n"
    "'R' - software reset\n"
    "'T' - start Temperature measurement\n"
    "'W' - test watchdog\n"
;

const char *parse_cmd(const char *buf){
    uint8_t u;
    if(buf[1] == '\n'){ // one symbol commands
        switch(*buf){
            case '0':
                si7005_setup();
                return "Reset I2C\n";
            break;
            /*case 'p':
                pin_toggle(USBPU_port, USBPU_pin);
                USB_send("USB pullup is ");
                if(pin_read(USBPU_port, USBPU_pin)) USND("off\n");
                else USB_send("on\n");
                return NULL;
            break;*/
            case 'I':
                if(si7005_read_ID(&u)){
                    USB_send("devid=");
                    USB_send(u2str(u));
                    USB_send("\n");
                }else USB_send("Can't read ID\n");
            break;
            case 'H':
                if(si7005_cmdH()) USB_send("Humid. read error\n");
            break;
            case 'R':
                USB_send("Soft reset\n");
                NVIC_SystemReset();
            break;
            case 'T':
                if(si7005_cmdT()) USB_send("Temper. read error\n");
                //USB_send(u2str(Tms)); USB_send("\n");
            break;
            case 'W':
                USB_send("Wait for reboot\n");
                while(1){nop();};
            break;
            default:
                return helpmsg;
        }
        return NULL;
    }
    uint32_t Num = 0;
    int32_t I;
    char *nxt;
    switch(*buf){ // long messages
        case 'N':
            ++buf;
            nxt = getnum(buf, &Num);
            if(buf == nxt){
                if(Num == 0) return "Wrong number\n";
                return "Integer32 overflow\n";
            }
            USB_send("ln("); USB_send(u2str(Num)); USB_send(")=");
            I = ln(Num);
            if(I < 0){USB_send("-"); I = -I;}
            USB_send(u2str(I));
            if(*nxt && *nxt != '\n'){
                USB_send(", the rest of string: ");
                USB_send(nxt);
            }else USB_send("\n");
        break;
        default:
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
