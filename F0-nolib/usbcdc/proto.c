/*
 *                                                                                                  geany_encoding=koi8-r
 * proto.c
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "can.h"
#include "hardware.h"
#include "proto.h"
#include "usart.h"
#include "usb.h"

#include <string.h> // strlen

extern volatile uint8_t canerror;

static char buff[UARTBUFSZ+1], *bptr = buff;
static uint8_t blen = 0, USBcmd = 0;

void sendbuf(){
    IWDG->KR = IWDG_REFRESH;
    if(blen == 0) return;
    *bptr = 0;
    if(USBcmd) USB_sendstr(buff);
    else{
        usart_send(buff);
        transmit_tbuf();
    }
    bptr = buff;
    blen = 0;
}

void switchbuff(uint8_t isUSB){
    USBcmd = isUSB;
}

/*
void memcpy(void *dest, const void *src, int len){
    while(len > 4){
        *(uint32_t*)dest++ = *(uint32_t*)src++;
        len -= 4;
    }
    while(len--) *(uint8_t*)dest++ = *(uint8_t*)src++;
}

int strlen(const char *txt){
    int l = 0;
    while(*txt++) ++l;
    return l;
}*/

void bufputchar(char ch){
    if(blen > UARTBUFSZ-1){
        sendbuf();
    }
    *bptr++ = ch;
    ++blen;
}

void addtobuf(const char *txt){
    IWDG->KR = IWDG_REFRESH;
    while(*txt) bufputchar(*txt++);
}

char *omit_spaces(char *buf){
    while(*buf){
        if(*buf > ' ') break;
        ++buf;
    }
    return buf;
}

// THERE'S NO OVERFLOW PROTECTION IN NUMBER READ PROCEDURES!
// read decimal number
static char *getdec(char *buf, uint32_t *N){
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '9'){
            break;
        }
        num *= 10;
        num += c - '0';
        ++buf;
    }
    *N = num;
    return buf;
}
// read hexadecimal number (without 0x prefix!)
static char *gethex(char *buf, uint32_t *N){
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
    *N = num;
    return buf;
}
// read binary number (without 0b prefix!)
static char *getbin(char *buf, uint32_t *N){
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
    *N = num;
    return buf;
}

/**
 * @brief getnum - read uint32_t from string (dec, hex or bin: 127, 0x7f, 0b1111111)
 * @param buf - buffer with number and so on
 * @param N   - the number read
 * @return pointer to first non-number symbol in buf (if it is == buf, there's no number)
 */
char *getnum(char *txt, uint32_t *N){
    if(*txt == '0'){
        if(txt[1] == 'x' || txt[1] == 'X') return gethex(txt+2, N);
        if(txt[1] == 'b' || txt[1] == 'B') return getbin(txt+2, N);
    }
    return getdec(txt, N);
}


// send command, format: ID (hex/bin/dec) data bytes (up to 8 bytes, space-delimeted)
void sendCANcommand(char *txt){
    SEND("CAN command with arguments:\n");
    uint32_t N;
    char *n;
    do{
        txt = omit_spaces(txt);
        n = getnum(txt, &N);
        if(txt == n) break;
        printu(N); SEND(", hex: ");
        printuhex(N); newline();
        txt = n;
    }while(1);
    if(*n){
        SEND("\nThe rest: ");
        SEND(n);
    }
    newline();
    sendbuf();
}

/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 * @param isUSB - == 1 if data got from USB
 */
void cmd_parser(char *txt, uint8_t isUSB){
    USBcmd = isUSB;
    //int16_t L = (int16_t)strlen(txt);
    char _1st = txt[0];
    /*
     * parse long commands here
     */
    if(_1st == 's' || _1st == 'S'){
        sendCANcommand(txt + 1);
        return;
    }
    if(txt[1] != '\n') *txt = '?'; // help for wrong message length
    switch(_1st){
        case 'f':
            transmit_tbuf();
        break;
        case 'B':
            can_send_broadcast();
        break;
        case 'C':
            can_send_dummy();
        break;
        case 'G':
            SEND("Can address: ");
            printuhex(getCANID());
            newline();
        break;
        case 'I':
            CAN_reinit();
            SEND("Can address: ");
            printuhex(getCANID());
            newline();
        break;
        case 'R':
            SEND("Soft reset\n");
            sendbuf();
            pause_ms(5); // a little pause to transmit data
            NVIC_SystemReset();
        break;
        case 'T':
            SEND("Time (ms): ");
            printu(Tms);
            newline();
        break;
        case 'U':
            USND("Test string for USB; a very long string that don't fit into one 64-byte buffer, what will be with it?\n");
        break;
        case 'W':
            SEND("Test watchdog\n");
            sendbuf();
            pause_ms(5); // a little pause to transmit data
            while(1){nop();}
        break;
        default: // help
            SEND(
            "'B' - send broadcast dummy byte\n"
            "'C' - send dummy byte over CAN\n"
            "'f' - flush UART buffer\n"
            "'G' - get CAN address\n"
            "'I' - reinit CAN (with new address)\n"
            "'R' - software reset\n"
            "'s/S' - send data over CAN: s ID byte0 .. byteN\n"
            "'T' - gen time from start (ms)\n"
            "'U' - send test string over USB\n"
            "'W' - test watchdog\n"
            );
        break;
    }
    sendbuf();
}

// print 32bit unsigned int
void printu(uint32_t val){
    char buf[11], *bufptr = &buf[10];
    *bufptr = 0;
    if(!val){
        *(--bufptr) = '0';
    }else{
        while(val){
            *(--bufptr) = val % 10 + '0';
            val /= 10;
        }
    }
    addtobuf(bufptr);
}

// print 32bit unsigned int as hex
void printuhex(uint32_t val){
    addtobuf("0x");
    uint8_t *ptr = (uint8_t*)&val + 3;
    int i, j;
    for(i = 0; i < 4; ++i, --ptr){
        for(j = 1; j > -1; --j){
            uint8_t half = (*ptr >> (4*j)) & 0x0f;
            if(half < 10) bufputchar(half + '0');
            else bufputchar(half - 10 + 'a');
        }
    }
}
