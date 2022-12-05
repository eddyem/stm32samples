/*
 * This file is part of the canuart project.
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

#include "can.h"
#include "hardware.h"
#include "proto.h"
#include "usart.h"
#include "version.inc"

extern volatile uint8_t canerror;

uint8_t ShowMsgs = 07;
uint16_t Ignore_IDs[IGN_SIZE];
uint8_t IgnSz = 0;

char *omit_spaces(const char *buf){
    while(*buf){
        if(*buf > ' ') break;
        ++buf;
    }
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
    if(!*s) return (char*)txt;
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

// parse `txt` to CAN_message
static CAN_message *parseCANmsg(char *txt){
    static CAN_message canmsg;
    uint32_t N;
    char *n;
    int ctr = -1;
    canmsg.ID = 0xffff;
    do{
        n = getnum(txt, &N);
        if(txt == n) break;
        txt = n;
        if(ctr == -1){
            if(N > 0x7ff){
                usart_send("ID should be 11-bit number!\n");
                return NULL;
            }
            canmsg.ID = (uint16_t)(N&0x7ff);
            ctr = 0;
            continue;
        }
        if(ctr > 7){
            usart_send("ONLY 8 data bytes allowed!\n");
            return NULL;
        }
        if(N > 0xff){
            usart_send("Every data portion is a byte!\n");
            return NULL;
        }
        canmsg.data[ctr++] = (uint8_t)(N&0xff);
    }while(1);
    if(canmsg.ID == 0xffff){
        usart_send("NO ID given, send nothing!\n");
        return NULL;
    }
    usart_send("Message parsed OK\n");
    canmsg.length = (uint8_t) ctr;
    return &canmsg;
}

// usart_send command, format: ID (hex/bin/dec) data bytes (up to 8 bytes, space-delimeted)
TRUE_INLINE void usart_sendCANcommand(char *txt){
    if(CAN1->MSR & CAN_MSR_INAK){
        usart_send("CAN bus is off, try to restart it\n");
        return;
    }
    CAN_message *msg = parseCANmsg(txt);
    if(!msg) return;
    uint32_t N = 5;
    while(CAN_BUSY == can_send(msg->data, msg->length, msg->ID)){
        if(--N == 0) break;
    }
}

TRUE_INLINE void CANini(char *txt){
    uint32_t N;
    char *n = getnum(txt, &N);
    if(txt == n){
        usart_send("No speed given");
        return;
    }
    if(N < 50){
        usart_send("Lowest speed is 50kbps");
        return;
    }else if(N > 3000){
        usart_send("Highest speed is 3000kbps");
        return;
    }
    CAN_reinit((uint16_t)N);
    usart_send("Reinit CAN bus with speed ");
    printu(N); usart_send("kbps");
}

TRUE_INLINE void addIGN(char *txt){
    if(IgnSz == IGN_SIZE){
        usart_send("Ignore buffer is full");
        return;
    }
    uint32_t N;
    char *n = getnum(txt, &N);
    if(txt == n){
        usart_send("No ID given");
        return;
    }
    if(N > 0x7ff){
        usart_send("ID should be 11-bit number!");
        return;
    }
    Ignore_IDs[IgnSz++] = (uint16_t)(N & 0x7ff);
    usart_send("Added ID "); printu(N);
    usart_send("\nIgn buffer size: "); printu(IgnSz);
}

TRUE_INLINE void print_ign_buf(){
    if(IgnSz == 0){
        usart_send("Ignore buffer is empty");
        return;
    }
    usart_send("Ignored IDs:\n");
    for(int i = 0; i < IgnSz; ++i){
        printu(i);
        usart_send(": ");
        printuhex(Ignore_IDs[i]);
        usart_putchar('\n');
    }
}

// print ID/mask of CAN1->sFilterRegister[x] half
static void printID(uint16_t FRn){
    if(FRn & 0x1f) return; // trash
    printuhex(FRn >> 5);
}
/*
Can filtering: FSCx=0 (CAN1->FS1R) -> 16-bit identifiers
CAN1->FMR = (sb)<<8 | FINIT - init filter in starting bank sb
CAN1->FFA1R FFAx = 1 -> FIFO1, 0 -> FIFO0
CAN1->FA1R FACTx=1 - filter active
MASK: FBMx=0 (CAN1->FM1R), two filters (n in FR1 and n+1 in FR2)
    ID:   CAN1->sFilterRegister[x].FRn[0..15]
    MASK: CAN1->sFilterRegister[x].FRn[16..31]
    FR bits:  STID[10:0] RTR IDE EXID[17:15]
LIST: FBMx=1, four filters (n&n+1 in FR1, n+2&n+3 in FR2)
    IDn:   CAN1->sFilterRegister[x].FRn[0..15]
    IDn+1: CAN1->sFilterRegister[x].FRn[16..31]
*/
TRUE_INLINE void list_filters(){
    uint32_t fa = CAN1->FA1R, ctr = 0, mask = 1;
    while(fa){
        if(fa & 1){
            usart_send("Filter "); printu(ctr); usart_send(", FIFO");
            if(CAN1->FFA1R & mask) usart_send("1");
            else usart_send("0");
            usart_send(" in ");
            if(CAN1->FM1R & mask){ // up to 4 filters in LIST mode
                usart_send("LIST mode, IDs: ");
                printID(CAN1->sFilterRegister[ctr].FR1 & 0xffff);
                usart_send(" ");
                printID(CAN1->sFilterRegister[ctr].FR1 >> 16);
                usart_send(" ");
                printID(CAN1->sFilterRegister[ctr].FR2 & 0xffff);
                usart_send(" ");
                printID(CAN1->sFilterRegister[ctr].FR2 >> 16);
            }else{ // up to 2 filters in MASK mode
                usart_send("MASK mode: ");
                if(!(CAN1->sFilterRegister[ctr].FR1&0x1f)){
                    usart_send("ID="); printID(CAN1->sFilterRegister[ctr].FR1 & 0xffff);
                    usart_send(", MASK="); printID(CAN1->sFilterRegister[ctr].FR1 >> 16);
                    usart_send(" ");
                }
                if(!(CAN1->sFilterRegister[ctr].FR2&0x1f)){
                    usart_send("ID="); printID(CAN1->sFilterRegister[ctr].FR2 & 0xffff);
                    usart_send(", MASK="); printID(CAN1->sFilterRegister[ctr].FR2 >> 16);
                }
            }
            usart_putchar('\n');
        }
        fa >>= 1;
        ++ctr;
        mask <<= 1;
    }
}

/**
 * @brief add_filter - add/modify filter
 * @param str - string in format "bank# FIFO# mode num0 .. num3"
 * where bank# - 0..27
 * if there's nothing after bank# - delete filter
 * FIFO# - 0,1
 * mode - 'I' for ID, 'M' for mask
 * num0..num3 - IDs in ID mode, ID/MASK for mask mode
 */
static void add_filter(char *str){
    uint32_t N;
    char *n = getnum(str, &N);
    if(n == str){
        usart_send("No bank# given");
        return;
    }
    if(N > STM32F0FBANKNO-1){
        usart_send("bank# > 27");
        return;
    }
    uint8_t bankno = (uint8_t)N;
    str = omit_spaces(n);
    if(!*str){ // deactivate filter
        usart_send("Deactivate filters in bank ");
        printu(bankno);
        CAN1->FMR = CAN_FMR_FINIT;
        CAN1->FA1R &= ~(1<<bankno);
        CAN1->FMR &=~ CAN_FMR_FINIT;
        return;
    }
    uint8_t fifono = 0;
    if(*str == '1') fifono = 1;
    else if(*str != '0'){
        usart_send("FIFO# is 0 or 1");
        return;
    }
    str = omit_spaces(str + 1);
    char c = *str;
    uint8_t mode = 0; // ID
    if(c == 'M' || c == 'm') mode = 1;
    else if(c != 'I' && c != 'i'){
        usart_send("mode is 'M/m' for MASK and 'I/i' for IDLIST");
        return;
    }
    str = omit_spaces(str + 1);
    uint32_t filters[4];
    uint32_t nfilt;
    for(nfilt = 0; nfilt < 4; ++nfilt){
        n = getnum(str, &N);
        if(n == str) break;
        filters[nfilt] = N;
        str = omit_spaces(n);
    }
    if(nfilt == 0){
        usart_send("You should add at least one filter!");
        return;
    }
    if(mode && (nfilt&1)){
        usart_send("In MASK mode you should point pairs of ID/MASK");
        return;
    }
    CAN1->FMR = CAN_FMR_FINIT;
    uint32_t mask = 1<<bankno;
    CAN1->FA1R |= mask; // activate given filter
    if(fifono) CAN1->FFA1R |= mask; // set FIFO number
    else CAN1->FFA1R &= ~mask;
    if(mode) CAN1->FM1R &= ~mask; // MASK
    else CAN1->FM1R |= mask; // LIST
    uint32_t F1 = (0x8f<<16);
    uint32_t F2 = (0x8f<<16);
    // reset filter registers to wrong value
    CAN1->sFilterRegister[bankno].FR1 = (0x8f<<16) | 0x8f;
    CAN1->sFilterRegister[bankno].FR2 = (0x8f<<16) | 0x8f;
    switch(nfilt){
        case 4:
            F2 = filters[3] << 21;
            // fallthrough
        case 3:
            CAN1->sFilterRegister[bankno].FR2 = (F2 & 0xffff0000) | (filters[2] << 5);
            // fallthrough
        case 2:
            F1 = filters[1] << 21;
            // fallthrough
        case 1:
            CAN1->sFilterRegister[bankno].FR1 = (F1 & 0xffff0000) | (filters[0] << 5);
    }
    CAN1->FMR &=~ CAN_FMR_FINIT;
    usart_send("Added filter with ");
    printu(nfilt); usart_send(" parameters");
}

const char *helpmsg =
    "https://github.com/eddyem/stm32samples/tree/master/F1:F103/canUART build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
    "'a' - add ID to ignore list (max 10 IDs)\n"
    "'b' - reinit CAN with given baudrate\n"
    "'c' - get CAN status\n"
    "'d' - delete ignore list\n"
    "'e' - get CAN errcodes\n"
    "'f' - add/delete filter, format: bank# FIFO# mode(M/I) num0 [num1 [num2 [num3]]]\n"
    "'F' - usart_send/clear flood message: F ID byte0 ... byteN\n"
    "'I' - reinit CAN\n"
    "'l' - list all active filters\n"
    "'o' - turn LEDs OFF\n"
    "'O' - turn LEDs ON\n"
    "'p' - print ignore buffer\n"
    "'P' - pause/resume in packets displaying\n"
    "'R' - software reset\n"
    "'s/S' - usart_send data over CAN: s ID byte0 .. byteN\n"
    "'t' - change flood period (>=1ms)\n"
    "'T' - get time from start (ms)\n"
;

TRUE_INLINE void getcanstat(){
    usart_send("CAN_MSR=");
    printuhex(CAN1->MSR);
    usart_send("\nCAN_TSR=");
    printuhex(CAN1->TSR);
    usart_send("\nCAN_RF0R=");
    printuhex(CAN1->RF0R);
    usart_send("\nCAN_RF1R=");
    printuhex(CAN1->RF1R);
}

TRUE_INLINE void setfloodt(char *s){
    uint32_t N;
    char *n = getnum(s, &N);
    if(s == n || N == 0){
        usart_send("t="); printu(floodT); usart_putchar('\n');
        return;
    }
    floodT = N - 1;
}

/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 */
void cmd_parser(char *txt){
    char _1st = txt[0];
    ++txt;
    /*
     * parse long commands here
     */
    switch(_1st){
        case 'a':
            addIGN(txt);
            goto eof;
        break;
        case 'b':
            CANini(txt);
            goto eof;
        break;
        case 'f':
            add_filter(txt);
            goto eof;
        break;
        case 'F':
            set_flood(parseCANmsg(txt));
            goto eof;
        break;
        case 's':
        case 'S':
            usart_sendCANcommand(txt);
            goto eof;
        break;
        case 't':
            setfloodt(txt);
            goto eof;
        break;
    }
    if(txt[1] != 0) _1st = '?'; // help for wrong message length
    switch(_1st){
        case 'c':
            getcanstat();
        break;
        case 'd':
            IgnSz = 0;
        break;
        case 'e':
            printCANerr();
        break;
        case 'I':
            CAN_reinit(0);
        break;
        case 'l':
            list_filters();
        break;
        case 'o':
            ledsON = 0;
            LED_off(LED0);
            LED_off(LED1);
        break;
        case 'O':
            ledsON = 1;
        break;
        case 'p':
            print_ign_buf();
        break;
        case 'P':
            ShowMsgs = !ShowMsgs;
            if(ShowMsgs) usart_send("Resume\n");
            else usart_send("Pause\n");
        break;
        case 'R':
            usart_send("Soft reset\n");
            usart_transmit();
            // wait until DMA & USART done
            while(!usart_txrdy);
            while(!(USART1->SR & USART_SR_TXE));
            USART1->CR1 = 0; // stop USART
            NVIC_SystemReset();
        break;
        case 'T':
            usart_send("Time (ms): ");
            printu(Tms);
            usart_putchar('\n');
        break;
        default: // help
            usart_send(helpmsg);
        break;
    }
eof:
    usart_putchar('\n');
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
    usart_send(bufptr);
}

// print 32bit unsigned int as hex
void printuhex(uint32_t val){
    usart_send("0x");
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
            if(half < 10) usart_putchar(half + '0');
            else usart_putchar(half - 10 + 'a');
        }
    }
}

// check Ignore_IDs & return 1 if ID isn't in list
uint8_t isgood(uint16_t ID){
    for(int i = 0; i < IgnSz; ++i)
        if(Ignore_IDs[i] == ID) return 0;
    return 1;
}
