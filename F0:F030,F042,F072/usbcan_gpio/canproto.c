/*
 * This file is part of the usbcangpio project.
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

#include <stm32f0.h>

#include "can.h"
#include "hardware.h"
#include "canproto.h"

#define USBIF   ICAN
#include "strfunc.h"

#define IGN_SIZE 10

extern volatile uint8_t canerror;

static uint8_t ShowMsgs = 1;
static uint16_t Ignore_IDs[IGN_SIZE];
static uint8_t IgnSz = 0;

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
                SEND("ID should be 11-bit number!\n");
                return NULL;
            }
            canmsg.ID = (uint16_t)(N&0x7ff);
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
    canmsg.length = (uint8_t) ctr;
    return &canmsg;
}

// USB_sendstr command, format: ID (hex/bin/dec) data bytes (up to 8 bytes, space-delimeted)
TRUE_INLINE void USB_sendstrCANcommand(char *txt){
    if(CAN->MSR & CAN_MSR_INAK){
        SEND("CAN bus is off, try to restart it\n");
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
        printu(CAN_getspeed());
        SEND("kbps\n");
        return;
    }
    if(N < CAN_MIN_SPEED){
        SEND("Speed is too low\n");
        return;
    }else if(N > CAN_MAX_SPEED){
        SEND("Speed is too high\n");
        return;
    }
    CAN_reinit((uint16_t)N);
    SEND("Reinit CAN bus with speed ");
    printu(N); SEND("kbps\n");
}

TRUE_INLINE void addIGN(char *txt){
    if(IgnSz == IGN_SIZE){
        SEND("Ignore buffer is full\n");
        return;
    }
    txt = omit_spaces(txt);
    uint32_t N;
    char *n = getnum(txt, &N);
    if(txt == n){
        SEND("No ID given\n");
        return;
    }
    if(N > 0x7ff){
        SEND("ID should be 11-bit number!\n");
        return;
    }
    Ignore_IDs[IgnSz++] = (uint16_t)(N & 0x7ff);
    SEND("Added ID "); printu(N);
    SEND("\nIgn buffer size: "); printu(IgnSz);
    NL();
}

TRUE_INLINE void print_ign_buf(){
    if(IgnSz == 0){
        SEND("Ignore buffer is empty\n");
        return;
    }
    SEND("Ignored IDs:\n");
    for(int i = 0; i < IgnSz; ++i){
        printu(i);
        SEND(": ");
        printuhex(Ignore_IDs[i]);
        NL();
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
            NL();
        }
        fa >>= 1;
        ++ctr;
        mask <<= 1;
    }
}

TRUE_INLINE void setfloodt(char *s){
    uint32_t N;
    s = omit_spaces(s);
    char *n = getnum(s, &N);
    if(s != n){
        floodT = N;
    }
    SEND("t="); printu(floodT); NL();
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
        SEND("No bank# given\n");
        return;
    }
    if(N > STM32F0FBANKNO-1){
        SEND("bank# > 27\n");
        return;
    }
    uint8_t bankno = (uint8_t)N;
    str = omit_spaces(n);
    if(!*str){ // deactivate filter
        SEND("Deactivate filters in bank ");
        printu(bankno); NL();
        CAN->FMR = CAN_FMR_FINIT;
        CAN->FA1R &= ~(1<<bankno);
        CAN->FMR &=~ CAN_FMR_FINIT;
        return;
    }
    uint8_t fifono = 0;
    if(*str == '1') fifono = 1;
    else if(*str != '0'){
        SEND("FIFO# is 0 or 1\n");
        return;
    }
    str = omit_spaces(str + 1);
    char c = *str;
    uint8_t mode = 0; // ID
    if(c == 'M' || c == 'm') mode = 1;
    else if(c != 'I' && c != 'i'){
        SEND("mode is 'M/m' for MASK and 'I/i' for IDLIST\n");
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
        SEND("You should add at least one filter!\n");
        return;
    }
    if(mode && (nfilt&1)){
        SEND("In MASK mode you should point pairs of ID/MASK\n");
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
    printu(nfilt); SEND(" parameters\n");
}

static const char *helpmsg =
    REPOURL
    "'a' - add ID to ignore list (max 10 IDs)\n"
    "'b' - reinit CAN with given baudrate or get current\n"
    "'c' - get CAN status\n"
    "'d' - delete ignore list\n"
#ifdef STM32F072xB
    "'D' - activate DFU mode\n"
#endif
    "'e' - get CAN errcodes\n"
    "'f' - add/delete filter, format: bank# FIFO# mode(M/I) num0 [num1 [num2 [num3]]]\n"
    "'F' - send/clear flood message: F ID byte0 ... byteN\n"
    "'i' - send incremental flood message (ID == ID for `F`)\n"
    "'I' - reinit CAN\n"
    "'l' - list all active filters\n"
    "'o' - turn LEDs OFF\n"
    "'O' - turn LEDs ON\n"
    "'p' - print ignore buffer\n"
    "'P' - pause/resume in packets displaying\n"
    "'R' - software reset\n"
    "'s/S' - send data over CAN: s ID byte0 .. byteN\n"
    "'t' - change flood period (>=0ms)\n"
    "'T' - get time from start (ms)\n"
;

TRUE_INLINE void getcanstat(){
    SEND("CAN_MSR=");
    printuhex(CAN->MSR);
    SEND("\nCAN_TSR=");
    printuhex(CAN->TSR);
    SEND("\nCAN_RF0R=");
    printuhex(CAN->RF0R);
    SEND("\nCAN_RF1R=");
    printuhex(CAN->RF1R);
    NL();
}

/**
 * @brief CommandParser - command parsing
 * @param txt   - buffer with commands & data
 * @param isUSB - == 1 if data got from USB
 */
static void CommandParser(char *txt){
    char _1st = txt[0];
    ++txt;
    /*
     * parse long commands here
     */
    switch(_1st){
        case 'a':
            addIGN(txt);
            return;
        break;
        case 'b':
            CANini(txt);
            return;
        break;
        case 'f':
            add_filter(txt);
            return;
        break;
        case 'F':
            set_flood(parseCANmsg(txt), 0);
            return;
        break;
        case 's':
        case 'S':
            USB_sendstrCANcommand(txt);
            return;
        break;
        case 't':
            setfloodt(txt);
            return;
        break;
    }
    if(*txt) _1st = '?'; // help for wrong message length
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
#ifdef STM32F072xB
        case 'D':
            USB_sendstr("Go into DFU mode\n");
            USB_sendall();
            Jump2Boot();
        break;
#endif
        case 'i':
            set_flood(NULL, 1);
            SEND("Incremental flooding is ON ('F' to off)\n");
        break;
        case 'I':
            SEND("CANspeed=");
            printu(CAN_reinit(0));
            NL();
        break;
        case 'l':
            list_filters();
        break;
        case 'o':
            ledsON = 0;
            LED_off(LED0);
            LED_off(LED1);
            SEND("LEDS=0\n");
        break;
        case 'O':
            ledsON = 1;
            SEND("LEDS=1\n");
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
            USB_sendall(ICAN); // wait until everything will gone
            NVIC_SystemReset();
        break;
        case 'T':
            SEND("Time (ms): ");
            printu(Tms);
            NL();
        break;
        default: // help
            SEND(helpmsg);
        break;
    }
}

// check Ignore_IDs & return 1 if ID isn't in list
static uint8_t isgood(uint16_t ID){
    for(int i = 0; i < IgnSz; ++i)
        if(Ignore_IDs[i] == ID) return 0;
    return 1;
}

void CANUSB_process(){
    char inbuff[MAXSTRLEN];
    CAN_message *can_mesg;
    uint32_t lastT = 0;
    can_proc();
    if(CAN_get_status() == CAN_FIFO_OVERRUN){
        SEND("CAN bus fifo overrun occured!\n");
    }
    while((can_mesg = CAN_messagebuf_pop())){
        IWDG->KR = IWDG_REFRESH;
        if(can_mesg && isgood(can_mesg->ID)){
            LED_on(LED0);
            lastT = Tms;
            if(!lastT) lastT = 1;
            if(ShowMsgs){ // new data in buff
                IWDG->KR = IWDG_REFRESH;
                uint8_t len = can_mesg->length;
                printu(Tms);
                SEND(" #");
                printuhex(can_mesg->ID);
                for(uint8_t ctr = 0; ctr < len; ++ctr){
                    PUTCHAR(' ');
                    printuhex(can_mesg->data[ctr]);
                }
                NL();
            }
        }
    }
    if(lastT && (Tms - lastT > 199)){
        LED_off(LED0);
        lastT = 0;
    }
    int l = RECV(inbuff, MAXSTRLEN);
    if(l < 0) SEND("ERROR: USB buffer overflow or string was too long\n");
    else if(l) CommandParser(inbuff);
}
