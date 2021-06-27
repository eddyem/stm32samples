/*
 * This file is part of the F0testbrd project.
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

#include "adc.h"
#include "proto.h"
#include "usart.h"
#include "usb.h"
#include "usb_lib.h"

void USB_sendstr(const char *str){
    uint16_t l = 0;
    const char *b = str;
    while(*b++) ++l;
    USB_send((const uint8_t*)str, l);
}

static char *chPWM(volatile uint32_t *reg, char *buf){
    char *lbuf = buf;
    lbuf = omit_spaces(lbuf);
    char cmd = *lbuf;
    lbuf = omit_spaces(lbuf + 1);
    uint32_t N;
    if(getnum(lbuf, &N) == lbuf) N = 1;
    uint32_t oldval = *reg;
    if(cmd == '-'){ // decrement
        if(oldval < N) return "Already at minimum";
        else *reg -= N;
    }else if(cmd == '+'){ // increment
        if(oldval + N > 255) return "Already at maximum";
        else *reg += N;
    }else{
        USND("Wrong command: ");
        return buf;
    }
    return "OK";
}

static char *TIM3pwm(char *buf){
    uint8_t channel = *buf - '1';
    if(channel > 3) return "Wrong channel number";
    volatile uint32_t *reg = &TIM3->CCR1;
    return chPWM(&reg[channel], buf+1);
}

static char *getPWMvals(){
    USND("TIM1CH1: "); USB_sendstr(u2str(TIM1->CCR1));
    USND("\nTIM3CH1: "); USB_sendstr(u2str(TIM3->CCR1));
    USND("\nTIM3CH2: "); USB_sendstr(u2str(TIM3->CCR2));
    USND("\nTIM3CH3: "); USB_sendstr(u2str(TIM3->CCR3));
    USND("\nTIM3CH4: "); USB_sendstr(u2str(TIM3->CCR4));
    USND("\n");
    return NULL;
}

static char *USARTsend(char *buf){
    uint32_t N;
    if(buf == getnum(buf, &N)) return "Point number of USART";
    if(N < 1 || N > USARTNUM) return "Wrong USART number";
    buf = omit_spaces(buf + 1);
    usart_send(N, buf);
    transmit_tbuf();
    return "OK";
}

const char *helpstring =
        "'+'/'-'[num] - increase/decrease TIM1ch1 PWM by 1 or `num`\n"
        "1..4'+'/'-'[num] - increase/decrease TIM3chN PWM by 1 or `num`\n"
        "'g' - get PWM values\n"
        "'A' - get ADC values\n"
        "'L' - send long string over USB\n"
        "'R' - software reset\n"
        "'S' - send short string over USB\n"
        "'Ux' - send string to USARTx (1..3)\n"
        "'T' - MCU temperature\n"
        "'V' - Vdd\n"
        "'W' - test watchdog\n"
;

const char *parse_cmd(char *buf){
    // "long" commands
    switch(*buf){
        case '+':
        case '-':
            return chPWM(&TIM1->CCR1, buf);
        break;
        case '1':
        case '2':
        case '3':
        case '4':
            return TIM3pwm(buf);
        case 'U':
            return USARTsend(buf + 1);
        break;
    }
    // "short" commands
    if(buf[1] != '\n') return buf;
    switch(*buf){
        case 'g':
            return getPWMvals();
        break;
        case 'A':
            USND("ADC0: "); USB_sendstr(u2str(getADCval(0)));
            USND(" ("); USB_sendstr(u2str(getADCvoltate(0)));
            USND("/100 V)\nADC1: "); USB_sendstr(u2str(getADCval(1)));
            USND(" ("); USB_sendstr(u2str(getADCvoltate(1)));
            USND("/100 V)\n");
        break;
        case 'L':
            USND("Very long test string for USB (it's length is more than 64 bytes).\n"
                     "This is another part of the string! Can you see all of this?\n");
            return "Long test sent";
        break;
        case 'R':
            USND("Soft reset\n");
            //SEND("Soft reset\n");
            NVIC_SystemReset();
        break;
        case 'S':
            USND("Test string for USB\n");
            return "Short test sent";
        break;
        case 'T':
            return u2str(getMCUtemp());
        break;
        case 'V':
            return u2str(getVdd());
        break;
        case 'W':
            USND("Wait for reboot\n");
            //SEND("Wait for reboot\n");
            while(1){nop();};
        break;
        default: // help
            return helpstring;
        break;
    }
    return NULL;
}

// usb getline
char *get_USB(){
    static char tmpbuf[129], *curptr = tmpbuf;
    static int rest = 128;
    int x = USB_receive((uint8_t*)curptr);
    curptr[x] = 0;
    if(!x) return NULL;
    if(curptr[x-1] == '\n'){
        curptr = tmpbuf;
        rest = 128;
        return tmpbuf;
    }
    curptr += x; rest -= x;
    if(rest <= 0){ // buffer overflow
        curptr = tmpbuf;
        rest = 128;
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
// print 32bit unsigned int as hex
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
