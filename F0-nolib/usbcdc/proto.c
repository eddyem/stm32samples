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

uint16_t Ignore_IDs[IGN_SIZE];
uint8_t IgnSz = 0;
static char buff[UARTBUFSZ+1], *bptr = buff;
static uint8_t blen = 0, USBcmd = 0;

void sendbuf(){
    IWDG->KR = IWDG_REFRESH;
    if(blen == 0) return;
    *bptr = 0;
    if(USBcmd) USB_sendstr(buff);
    if(USBcmd != 1){
        usart_send(buff);
        transmit_tbuf();
    }
    bptr = buff;
    blen = 0;
}

// 1 - USB, 0 - USART, other number - both
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
TRUE_INLINE void sendCANcommand(char *txt){
    SEND("CAN command with arguments:\n");
    uint32_t N;
    uint16_t ID = 0xffff;
    char *n;
    uint8_t data[8];
    int ctr = -1;
    do{
        txt = omit_spaces(txt);
        n = getnum(txt, &N);
        if(txt == n) break;
        txt = n;
        if(ctr == -1){
            if(N > 0x7ff){
                SEND("ID should be 11-bit number!");
                return;
            }
            ID = (uint16_t)(N&0x7ff);
            SEND("ID="); printuhex(ID); newline();
            ctr = 0;
            continue;
        }
        if(ctr > 7){
            SEND("ONLY 8 data bytes allowed!");
            return;
        }
        if(N > 0xff){
            SEND("Every data portion is a byte!");
            return;
        }
        data[ctr++] = (uint8_t)(N&0xff);
        printu(N); SEND(", hex: ");
        printuhex(N); newline();
    }while(1);
    if(*n){
        SEND("\nUnusefull data: ");
        SEND(n);
    }
    if(ID == 0xffff){
        SEND("NO ID given, send nothing!");
        return;
    }
    sendbuf();
    N = 1000000;
    while(CAN_BUSY == can_send(data, (uint8_t)ctr, ID)){
        if(--N == 0) break;
    }
}

TRUE_INLINE void CANini(char *txt){
    txt = omit_spaces(txt);
    uint32_t N;
    char *n = getnum(txt, &N);
    if(txt == n){
        SEND("No speed given");
        return;
    }
    if(N < 50){
        SEND("Lowest speed is 50kbps");
        return;
    }else if(N > 3000){
        SEND("Highest speed is 3000kbps");
        return;
    }
    CAN_reinit((uint16_t)N);
    SEND("Reinit CAN bus with speed ");
    printu(N); SEND("kbps");
}

TRUE_INLINE void addIGN(char *txt){
    if(IgnSz == IGN_SIZE){
        MSG("Ignore buffer is full");
        return;
    }
    txt = omit_spaces(txt);
    uint32_t N;
    char *n = getnum(txt, &N);
    if(txt == n){
        SEND("No ID given");
        return;
    }
    if(N > 0x7ff){
        SEND("ID should be 11-bit number!");
        return;
    }
    Ignore_IDs[IgnSz++] = (uint16_t)(N & 0x7ff);
    SEND("Added ID "); printu(N);
    SEND("\nIgn buffer size: "); printu(IgnSz);
}

TRUE_INLINE void print_ign_buf(){
    if(IgnSz == 0){
        SEND("Ignore buffer is empty");
        return;
    }
    for(int i = 0; i < IgnSz; ++i){
        printu(i);
        SEND(": ");
        printuhex(Ignore_IDs[i]);
        newline();
    }
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
    switch(_1st){
        case 's':
        case 'S':
            sendCANcommand(txt + 1);
            goto eof;
        break;
        case 'b':
            CANini(txt + 1);
            goto eof;
        break;
        case 'a':
            addIGN(txt + 1);
            goto eof;
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
        case 'd':
            IgnSz = 0;
        break;
        case 'G':
            SEND("Can address: ");
            printuhex(getCANID());
            newline();
        break;
        case 'I':
            CAN_reinit(0);
            SEND("Can address: ");
            printuhex(getCANID());
            newline();
        break;
        case 'p':
            print_ign_buf();
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
            "'a' - add ID to ignore list (max 10 IDs)\n"
            "'b' - reinit CAN with given baudrate\n"
            "'B' - send broadcast dummy byte\n"
            "'C' - send dummy byte over CAN\n"
            "'d' - delete ignore list\n"
            "'f' - flush UART buffer\n"
            "'G' - get CAN address\n"
            "'I' - reinit CAN (with new address)\n"
            "'p' - print ignore buffer\n"
            "'R' - software reset\n"
            "'s/S' - send data over CAN: s ID byte0 .. byteN\n"
            "'T' - gen time from start (ms)\n"
            "'U' - send test string over USB\n"
            "'W' - test watchdog\n"
            );
        break;
    }
eof:
    newline();
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
    int8_t i, j;
    for(i = 0; i < 4; ++i, --ptr){
        if(*ptr == 0 && i != 3) continue; // omit leading zeros
        for(j = 1; j > -1; --j){
            uint8_t half = (*ptr >> (4*j)) & 0x0f;
            if(half < 10) bufputchar(half + '0');
            else bufputchar(half - 10 + 'a');
        }
    }
}

// check Ignore_IDs & return 1 if ID isn't in list
uint8_t isgood(uint16_t ID){
    for(int i = 0; i < IgnSz; ++i)
        if(Ignore_IDs[i] == ID) return 0;
    return 1;
}
