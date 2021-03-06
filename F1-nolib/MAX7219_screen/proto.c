/*
 * This file is part of the MAX7219 project.
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

#include "hardware.h"
#include "proto.h"
#include "MAX7219.h"
#include "usb.h"

extern volatile uint32_t Tms;
extern uint32_t rollpause;
extern uint8_t roll;

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
    "'0'   - setup again\n"
    "'Cxy' - clear pixel @ x,y\n"
    "'Ii'  - set intensity to i\n"
    "'Pp'  - change roll pause to p ms (5..1000)"
    "'R'   - refresh screen\n"
    "'r'   - roll screen or stop it\n"
    "'Sxy' - set pixel @ x,y\n"
    "'T/t' - test on/off\n"
;

const char *parse_cmd(const char *buf){
    if(buf[1] == '\n'){ // short commands
        switch(*buf){
            case '0':
                MAX7219_setup();
                return "Reinit\n";
            break;
            case 'R':
                MAX7219_refresh();
                return "Refresh\n";
            break;
            case 'r':
                if(roll){
                    roll = 0;
                    return "Stop rolling\n";
                }else{
                    roll = 1;
                    return "Start rolling\n";
                }
            case 'T':
                MAX7219_test(1);
            break;
            case 't':
                MAX7219_test(0);
            break;
            default: // help
                return helpmsg;
            break;
        }
    }else{
        uint32_t x, y = 0xffff;
        char *nxt = getnum(buf+1, &x), *n;
        int setclr = -1;
        if(nxt != buf+1){
            ++nxt;
            n = getnum(nxt, &y);
            if(n == nxt){
                y = 0xffff;
            }else{
                if(y > 7 || x > 8*NSCREENS) y = 0xffff;
            }
        }
        switch(*buf){
            case 'C':
                setclr = 0;
            break;
            case 'I':
                if(x > 15) return "I should be < 16!\n";
                MAX7219_setintens(x);
                return "Done\n";
            case 'P':
                if(x < 5 || x > 1000) return "Bad pause length\n";
                rollpause = x - 1;
                return "Changed\n";
            break;
            case 'S':
                setclr = 1;
            break;
            default:
                return buf;
        }
        if(setclr != -1){
            if(y == 0xffff) return "Wrong coordinates\n";
            if(!MAX7219_point(x, y, setclr)) return "Error\n";
            else return "Done";
        }
    }
    return NULL;
}

/*
const char *parse_cmd(const char *buf){
    uint8_t u;
    if(buf[1] == '\n'){ // one symbol commands
        switch(*buf){
            case '0':
                if(BMP180_reset()) return "Reseted\n";
                else return "Error in reset\n";
            break;
            case '1':
                if(!BMP180_init())  return "Error in init process\n";
                return "OK, inited\n";
            break;
            case 'I':
                BMP180_read_ID(&u);
                if(u == BMP180_CHIP_ID) return "BMP180\n";
                else return "ID unknown\n";
            break;
            case 'M':
                if(!BMP180_start()) return "BMP180 not found or not inited\n";
                else return "Start\n";
            break;
            case 'R':
                USB_send("Soft reset\n");
                NVIC_SystemReset();
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
    switch(*buf){ // long messages
        case 'O':
            ++buf;
            if(buf != getnum(buf, &Num) && Num < BMP180_OVERSMAX){
                BMP180_setOS(Num);
                return "Changed\n";
            }else return "Wrong OS number";
        break;
        default:
            return buf;
    }
    return NULL;
}*/

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
