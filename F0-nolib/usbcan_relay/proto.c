/*
 * This file is part of the canrelay project.
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

#include "buttons.h"
#include "can.h"
#include "hardware.h"
#include "proto.h"
#include "usb.h"

#include <string.h> // strlen

extern volatile uint8_t canerror;

uint8_t ShowMsgs = 0;
uint16_t Ignore_IDs[IGN_SIZE];
uint8_t IgnSz = 0;
static char buff[BUFSZ+1], *bptr = buff;
static uint8_t blen = 0;

void sendbuf(){
    IWDG->KR = IWDG_REFRESH;
    if(blen == 0) return;
    *bptr = 0;
    USB_sendstr(buff);
    bptr = buff;
    blen = 0;
}

void bufputchar(char ch){
    if(blen > BUFSZ-1){
        sendbuf();
    }
    *bptr++ = ch;
    ++blen;
}

void addtobuf(const char *txt){
    IWDG->KR = IWDG_REFRESH;
    while(*txt) bufputchar(*txt++);
}

char *omit_spaces(const char *buf){
    while(*buf){
        if(*buf > ' ') break;
        ++buf;
    }
    return (char*) buf;
}

// THERE'S NO OVERFLOW PROTECTION IN NUMBER READ PROCEDURES!
// read decimal number
static char *getdec(const char *buf, uint32_t *N){
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
    return (char *)buf;
}
// read hexadecimal number (without 0x prefix!)
static char *gethex(const char *buf, uint32_t *N){
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
    return (char *)buf;
}
// read binary number (without 0b prefix!)
static char *getbin(const char *buf, uint32_t *N){
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
    return (char *)buf;
}

/**
 * @brief getnum - read uint32_t from string (dec, hex or bin: 127, 0x7f, 0b1111111)
 * @param buf - buffer with number and so on
 * @param N   - the number read
 * @return pointer to first non-number symbol in buf (if it is == buf, there's no number)
 */
char *getnum(const char *txt, uint32_t *N){
    if(*txt == '0'){
        if(txt[1] == 'x' || txt[1] == 'X') return gethex(txt+2, N);
        if(txt[1] == 'b' || txt[1] == 'B') return getbin(txt+2, N);
    }
    return getdec(txt, N);
}

// parse `txt` to CAN_message
static CAN_message *parseCANmsg(char *txt){
    static CAN_message canmsg;
    //SEND("CAN command with arguments:\n");
    uint32_t N;
    char *n;
    int ctr = -1;
    canmsg.ID = 0xffff;
    do{
        txt = omit_spaces(txt);
        n = getnum(txt, &N);
        if(txt == n) break;
        txt = n;
        if(ctr == -1){
            if(N > 0x7ff){
                SEND("ID should be 11-bit number!\n");
                return NULL;
            }
            canmsg.ID = (uint16_t)(N&0x7ff);
            //SEND("ID="); printuhex(canmsg.ID); newline();
            ctr = 0;
            continue;
        }
        if(ctr > 7){
            SEND("ONLY 8 data bytes allowed!\n");
            return NULL;
        }
        if(N > 0xff){
            SEND("Every data portion is a byte!\n");
            return NULL;
        }
        canmsg.data[ctr++] = (uint8_t)(N&0xff);
    }while(1);
    if(canmsg.ID == 0xffff){
        SEND("NO ID given, send nothing!\n");
        return NULL;
    }
    SEND("Message parsed OK\n");
    sendbuf();
    canmsg.length = (uint8_t) ctr;
    return &canmsg;
}

// send command, format: ID (hex/bin/dec) data bytes (up to 8 bytes, space-delimeted)
TRUE_INLINE void sendCANcommand(char *txt){
    CAN_message *msg = parseCANmsg(txt);
    if(!msg) return;
    uint32_t N = 1000000;
    while(CAN_BUSY == can_send(msg->data, msg->length, msg->ID)){
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
    if(N == CANID){
        SEND("You can't ignore self ID!");
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
    SEND("Ignored IDs:\n");
    for(int i = 0; i < IgnSz; ++i){
        printu(i);
        SEND(": ");
        printuhex(Ignore_IDs[i]);
        newline();
    }
}

// print ID/mask of CAN->sFilterRegister[x] half
static void printID(uint16_t FRn){
    if(FRn & 0x1f) return; // trash
    printuhex(FRn >> 5);
}
/*
Can filtering: FSCx=0 (CAN->FS1R) -> 16-bit identifiers
CAN->FMR = (sb)<<8 | FINIT - init filter in starting bank sb
CAN->FFA1R FFAx = 1 -> FIFO1, 0 -> FIFO0
CAN->FA1R FACTx=1 - filter active
MASK: FBMx=0 (CAN->FM1R), two filters (n in FR1 and n+1 in FR2)
    ID:   CAN->sFilterRegister[x].FRn[0..15]
    MASK: CAN->sFilterRegister[x].FRn[16..31]
    FR bits:  STID[10:0] RTR IDE EXID[17:15]
LIST: FBMx=1, four filters (n&n+1 in FR1, n+2&n+3 in FR2)
    IDn:   CAN->sFilterRegister[x].FRn[0..15]
    IDn+1: CAN->sFilterRegister[x].FRn[16..31]
*/
TRUE_INLINE void list_filters(){
    uint32_t fa = CAN->FA1R, ctr = 0, mask = 1;
    while(fa){
        if(fa & 1){
            SEND("Filter "); printu(ctr); SEND(", FIFO");
            if(CAN->FFA1R & mask) SEND("1");
            else SEND("0");
            SEND(" in ");
            if(CAN->FM1R & mask){ // up to 4 filters in LIST mode
                SEND("LIST mode, IDs: ");
                printID(CAN->sFilterRegister[ctr].FR1 & 0xffff);
                SEND(" ");
                printID(CAN->sFilterRegister[ctr].FR1 >> 16);
                SEND(" ");
                printID(CAN->sFilterRegister[ctr].FR2 & 0xffff);
                SEND(" ");
                printID(CAN->sFilterRegister[ctr].FR2 >> 16);
            }else{ // up to 2 filters in MASK mode
                SEND("MASK mode: ");
                if(!(CAN->sFilterRegister[ctr].FR1&0x1f)){
                    SEND("ID="); printID(CAN->sFilterRegister[ctr].FR1 & 0xffff);
                    SEND(", MASK="); printID(CAN->sFilterRegister[ctr].FR1 >> 16);
                    SEND(" ");
                }
                if(!(CAN->sFilterRegister[ctr].FR2&0x1f)){
                    SEND("ID="); printID(CAN->sFilterRegister[ctr].FR2 & 0xffff);
                    SEND(", MASK="); printID(CAN->sFilterRegister[ctr].FR2 >> 16);
                }
            }
            newline();
        }
        fa >>= 1;
        ++ctr;
        mask <<= 1;
    }
    sendbuf();
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
    str = omit_spaces(str);
    char *n = getnum(str, &N);
    if(n == str){
        SEND("No bank# given");
        return;
    }
    if(N == 0 || N > STM32F0FBANKNO-1){
        SEND("0 (reserved for self) < bank# < 28 (max bank# is 27)!!!");
        return;
    }
    uint8_t bankno = (uint8_t)N;
    str = omit_spaces(n);
    if(!*str){ // deactivate filter
        SEND("Deactivate filters in bank ");
        printu(bankno);
        CAN->FMR = CAN_FMR_FINIT;
        CAN->FA1R &= ~(1<<bankno);
        CAN->FMR &=~ CAN_FMR_FINIT;
        return;
    }
    uint8_t fifono = 0;
    if(*str == '1') fifono = 1;
    else if(*str != '0'){
        SEND("FIFO# is 0 or 1");
        return;
    }
    str = omit_spaces(str + 1);
    char c = *str;
    uint8_t mode = 0; // ID
    if(c == 'M' || c == 'm') mode = 1;
    else if(c != 'I' && c != 'i'){
        SEND("mode is 'M/m' for MASK and 'I/i' for IDLIST");
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
        SEND("You should add at least one filter!");
        return;
    }
    if(mode && (nfilt&1)){
        SEND("In MASK mode you should point pairs of ID/MASK");
        return;
    }
    CAN->FMR = CAN_FMR_FINIT;
    uint32_t mask = 1<<bankno;
    CAN->FA1R |= mask; // activate given filter
    if(fifono) CAN->FFA1R |= mask; // set FIFO number
    else CAN->FFA1R &= ~mask;
    if(mode) CAN->FM1R &= ~mask; // MASK
    else CAN->FM1R |= mask; // LIST
    uint32_t F1 = (0x8f<<16);
    uint32_t F2 = (0x8f<<16);
    // reset filter registers to wrong value
    CAN->sFilterRegister[bankno].FR1 = (0x8f<<16) | 0x8f;
    CAN->sFilterRegister[bankno].FR2 = (0x8f<<16) | 0x8f;
    switch(nfilt){
        case 4:
            F2 = filters[3] << 21;
            // fallthrough
        case 3:
            CAN->sFilterRegister[bankno].FR2 = (F2 & 0xffff0000) | (filters[2] << 5);
            // fallthrough
        case 2:
            F1 = filters[1] << 21;
            // fallthrough
        case 1:
            CAN->sFilterRegister[bankno].FR1 = (F1 & 0xffff0000) | (filters[0] << 5);
    }
    CAN->FMR &=~ CAN_FMR_FINIT;
    SEND("Added filter with ");
    printu(nfilt); SEND(" parameters");
}

/**
 * @brief ledsOp - turn on/off LEDs
 * @param str - LED number (0..3)
 * @param state = 1 to turn on, 0 to turn off (toggling only available for CAN bus commands)
 */
static void ledsOp(const char *str, uint8_t state){
    uint32_t N;
    char *x = getnum(str, &N);
    if(!x || x == str || N > LEDSNO - 1){
        SEND("Wrong LED number\n");
        return;
    }
    if(state) LED_on(N);
    else LED_off(N);
    SEND("LED"); printu(N); SEND(" is ");
    if(state) SEND("on"); else SEND("off");
}

// print current buttons state
TRUE_INLINE void getBtnState(){
    const char *states[] = {[EVT_NONE] = NULL, [EVT_PRESS] = "pressed", [EVT_HOLD] = "holded", [EVT_RELEASE] = "released"};
    for(int i = 0; i < BTNSNO; ++i){
        uint32_t T;
        keyevent e = keystate(i, &T);
        if(e != EVT_NONE){
            SEND("The key "); printu(i);
            SEND(" is "); addtobuf(states[e]); SEND(" at ");
            printu(T); NL();
        }
    }
}

TRUE_INLINE void getPWM(){
    volatile uint32_t *reg = &TIM1->CCR1;
    for(int n = 0; n < 3; ++n){
        SEND("PWM");
        bufputchar(n);
        bufputchar('=');
        printu(*reg++);
        bufputchar('\n');
    }
    sendbuf();
}

TRUE_INLINE void changePWM(char *str){
    str = omit_spaces(str);
    uint32_t N, pwm;
    char *nxt = getnum(str, &N);
    if(nxt == str || N > 2){
        SEND("Nch = 0..2");
        return;
    }
    str = omit_spaces(nxt);
    nxt = getnum(str, &pwm);
    if(nxt == str || pwm > 255){
        SEND("PWM should be from 0 to 255");
        return;
    }
    volatile uint32_t *reg = &TIM1->CCR1;
    reg[N] = pwm;
    SEND("OK, changed");
}

/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 * @param isUSB - == 1 if data got from USB
 */
void cmd_parser(char *txt){
    char _1st = txt[0];
    /*
     * parse long commands here
     */
    switch(_1st){
        case 'a':
            addIGN(txt + 1);
            goto eof;
        break;
        case 'C':
            CANini(txt + 1);
            goto eof;
        break;
        case 'f':
            add_filter(txt + 1);
            goto eof;
        break;
        case 'F':
            set_flood(parseCANmsg(txt + 1));
            goto eof;
        break;
        case 'o':
            ledsOp(txt + 1, 0);
            goto eof;
        break;
        case 'O':
            ledsOp(txt + 1, 1);
            goto eof;
        break;
        case 's':
        case 'S':
            sendCANcommand(txt + 1);
            goto eof;
        break;
        case 'W':
            changePWM(txt + 1);
            goto eof;
        break;
    }
    if(txt[1] != '\n') *txt = '?'; // help for wrong message length
    switch(_1st){
        case 'b':
            getBtnState();
        break;
        case 'd':
            IgnSz = 0;
        break;
        case 'D':
            SEND("Go into DFU mode\n");
            sendbuf();
            Jump2Boot();
        break;
        case 'I':
            SEND("CAN ID: "); printuhex(CANID);
        break;
        case 'l':
            list_filters();
        break;
        case 'p':
            print_ign_buf();
        break;
        case 'P':
            ShowMsgs = !ShowMsgs;
            if(ShowMsgs) SEND("Resume\n");
            else SEND("Pause\n");
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
        break;
        case 'w':
            getPWM();
            return;
        break;
        default: // help
            SEND(
            "'a' - add ID to ignore list (max 10 IDs)\n"
            "'b' - get buttons' state\n"
            "'C' - reinit CAN with given baudrate\n"
            "'d' - delete ignore list\n"
            "'D' - activate DFU mode\n"
            "'f' - add/delete filter, format: bank# FIFO# mode(M/I) num0 [num1 [num2 [num3]]]\n"
            "'F' - send/clear flood message: F ID byte0 ... byteN\n"
            "'I' - read CAN ID\n"
            "'l' - list all active filters\n"
            "'o' - turn nth LED OFF\n"
            "'O' - turn nth LED ON\n"
            "'p' - print ignore buffer\n"
            "'P' - pause/resume in packets displaying\n"
            "'R' - software reset\n"
            "'s/S' - send data over CAN: s ID byte0 .. byteN\n"
            "'T' - get time from start (ms)\n"
            "'w' - get PWM settings\n"
            "'W' - set PWM @nth channel (ch: 0..2, PWM: 0..255)\n"
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
    int8_t i, j, z=1;
    for(i = 0; i < 4; ++i, --ptr){
        if(*ptr == 0){ // omit leading zeros
            if(i == 3) z = 0;
            if(z) continue;
        }
        else z = 0;
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
