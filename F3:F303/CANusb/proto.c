/*
 * This file is part of the canusb project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32f3.h>
#include <string.h>

#include "can.h"
#include "hardware.h"
#include "proto.h"
#include "version.inc"

// software ignore buffer size
#define IGN_SIZE 10

extern volatile uint32_t Tms;

uint8_t ShowMsgs = 1;
// software ignore buffers
static uint16_t Ignore_IDs[IGN_SIZE];
static uint8_t IgnSz = 0;

// parse `txt` to CAN_message
static CAN_message *parseCANmsg(const char *txt){
    static CAN_message canmsg;
    uint32_t N;
    int ctr = -1;
    canmsg.ID = 0xffff;
    do{
        const char *n = getnum(txt, &N);
        if(txt == n) break;
        txt = n;
        if(ctr == -1){
            if(N > 0x7ff){
                USB_sendstr("ID should be 11-bit number!\n");
                return NULL;
            }
            canmsg.ID = (uint16_t)(N&0x7ff);
            ctr = 0;
            continue;
        }
        if(ctr > 7){
            USB_sendstr("ONLY 8 data bytes allowed!\n");
            return NULL;
        }
        if(N > 0xff){
            USB_sendstr("Every data portion is a byte!\n");
            return NULL;
        }
        canmsg.data[ctr++] = (uint8_t)(N&0xff);
    }while(1);
    if(canmsg.ID == 0xffff){
        USB_sendstr("NO ID given, send nothing!\n");
        return NULL;
    }
    USB_sendstr("Message parsed OK\n");
    canmsg.length = (uint8_t) ctr;
    return &canmsg;
}

// USB_sendstr command, format: ID (hex/bin/dec) data bytes (up to 8 bytes, space-delimeted)
TRUE_INLINE void USB_sendstrCANcommand(char *txt){
    if(CAN->MSR & CAN_MSR_INAK){
        USB_sendstr("CAN bus is off, try to restart it\n");
        return;
    }
    CAN_message *msg = parseCANmsg(txt);
    if(!msg) return;
    uint32_t N = 5;
    while(CAN_BUSY == can_send(msg->data, msg->length, msg->ID)){
        if(--N == 0) break;
    }
}

TRUE_INLINE void CANini(const char *txt){
    uint32_t N;
    const char *n = getnum(txt, &N);
    if(txt == n){
        USB_sendstr("No speed given");
        return;
    }
    if(N < 50){
        USB_sendstr("Lowest speed is 50kbps");
        return;
    }else if(N > 3000){
        USB_sendstr("Highest speed is 3000kbps");
        return;
    }
    CAN_reinit((uint16_t)N);
    USB_sendstr("Reinit CAN bus with speed ");
    printu(N); USB_sendstr("kbps");
}

TRUE_INLINE void addIGN(const char *txt){
    if(IgnSz == IGN_SIZE){
        USB_sendstr("Ignore buffer is full");
        return;
    }
    txt = omit_spaces(txt);
    uint32_t N;
    const char *n = getnum(txt, &N);
    if(txt == n){
        USB_sendstr("No ID given");
        return;
    }
    if(N > 0x7ff){
        USB_sendstr("ID should be 11-bit number!");
        return;
    }
    Ignore_IDs[IgnSz++] = (uint16_t)(N & 0x7ff);
    USB_sendstr("Added ID "); printu(N);
    USB_sendstr("\nIgn buffer size: "); printu(IgnSz);
}

TRUE_INLINE void print_ign_buf(){
    if(IgnSz == 0){
        USB_sendstr("Ignore buffer is empty");
        return;
    }
    USB_sendstr("Ignored IDs:\n");
    for(int i = 0; i < IgnSz; ++i){
        printu(i);
        USB_sendstr(": ");
        printuhex(Ignore_IDs[i]);
        USB_putbyte('\n');
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
            USB_sendstr("Filter "); printu(ctr); USB_sendstr(", FIFO");
            if(CAN->FFA1R & mask) USB_sendstr("1");
            else USB_sendstr("0");
            USB_sendstr(" in ");
            if(CAN->FM1R & mask){ // up to 4 filters in LIST mode
                USB_sendstr("LIST mode, IDs: ");
                printID(CAN->sFilterRegister[ctr].FR1 & 0xffff);
                USB_sendstr(" ");
                printID(CAN->sFilterRegister[ctr].FR1 >> 16);
                USB_sendstr(" ");
                printID(CAN->sFilterRegister[ctr].FR2 & 0xffff);
                USB_sendstr(" ");
                printID(CAN->sFilterRegister[ctr].FR2 >> 16);
            }else{ // up to 2 filters in MASK mode
                USB_sendstr("MASK mode: ");
                if(!(CAN->sFilterRegister[ctr].FR1&0x1f)){
                    USB_sendstr("ID="); printID(CAN->sFilterRegister[ctr].FR1 & 0xffff);
                    USB_sendstr(", MASK="); printID(CAN->sFilterRegister[ctr].FR1 >> 16);
                    USB_sendstr(" ");
                }
                if(!(CAN->sFilterRegister[ctr].FR2&0x1f)){
                    USB_sendstr("ID="); printID(CAN->sFilterRegister[ctr].FR2 & 0xffff);
                    USB_sendstr(", MASK="); printID(CAN->sFilterRegister[ctr].FR2 >> 16);
                }
            }
            USB_putbyte('\n');
        }
        fa >>= 1;
        ++ctr;
        mask <<= 1;
    }
}

TRUE_INLINE void setfloodt(const char *s){
    uint32_t N;
    s = omit_spaces(s);
    const char *n = getnum(s, &N);
    if(s == n){
        USB_sendstr("t="); printu(floodT); USB_putbyte('\n');
        return;
    }
    floodT = N;
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
static void add_filter(const char *str){
    uint32_t N;
    str = omit_spaces(str);
    const char *n = getnum(str, &N);
    if(n == str){
        USB_sendstr("No bank# given");
        return;
    }
    if(N > STM32F0FBANKNO-1){
        USB_sendstr("bank# > 27");
        return;
    }
    uint8_t bankno = (uint8_t)N;
    str = omit_spaces(n);
    if(!*str){ // deactivate filter
        USB_sendstr("Deactivate filters in bank ");
        printu(bankno);
        CAN->FMR = CAN_FMR_FINIT;
        CAN->FA1R &= ~(1<<bankno);
        CAN->FMR &=~ CAN_FMR_FINIT;
        return;
    }
    uint8_t fifono = 0;
    if(*str == '1') fifono = 1;
    else if(*str != '0'){
        USB_sendstr("FIFO# is 0 or 1");
        return;
    }
    str = omit_spaces(str + 1);
    const char c = *str;
    uint8_t mode = 0; // ID
    if(c == 'M' || c == 'm') mode = 1;
    else if(c != 'I' && c != 'i'){
        USB_sendstr("mode is 'M/m' for MASK and 'I/i' for IDLIST");
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
        USB_sendstr("You should add at least one filter!");
        return;
    }
    if(mode && (nfilt&1)){
        USB_sendstr("In MASK mode you should point pairs of ID/MASK");
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
    USB_sendstr("Added filter with ");
    printu(nfilt); USB_sendstr(" parameters");
}

const char *helpstring =
    "https://github.com/eddyem/stm32samples/tree/master/F3:F303/CANusb build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
    "'a' - add ID to ignore list (max 10 IDs)\n"
    "'b' - reinit CAN with given baudrate\n"
    "'c' - get CAN status\n"
    "'d' - delete ignore list\n"
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
    USB_sendstr("CAN_MSR=");
    printuhex(CAN->MSR);
    USB_sendstr("\nCAN_TSR=");
    printuhex(CAN->TSR);
    USB_sendstr("\nCAN_RF0R=");
    printuhex(CAN->RF0R);
    USB_sendstr("\nCAN_RF1R=");
    printuhex(CAN->RF1R);
}

/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 * @param isUSB - == 1 if data got from USB
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
            set_flood(parseCANmsg(txt), 0);
            goto eof;
        break;
        case 's':
        case 'S':
            USB_sendstrCANcommand(txt);
            goto eof;
        break;
        case 't':
            setfloodt(txt);
            goto eof;
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
        case 'i':
            set_flood(NULL, 1);
            USB_sendstr("Incremental flooding is ON ('F' to off)\n");
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
            if(ShowMsgs) USB_sendstr("Resume\n");
            else USB_sendstr("Pause\n");
        break;
        case 'R':
            USB_sendstr("Soft reset\n");
            USB_sendall(); // wait until everything will gone
            NVIC_SystemReset();
        break;
        case 'T':
            USB_sendstr("Time (ms): ");
            printu(Tms);
            USB_putbyte('\n');
        break;
        default: // help
            USB_sendstr(helpstring);
        break;
    }
eof:
    USB_putbyte('\n');
}

// check Ignore_IDs & return 1 if ID isn't in list
uint8_t isgood(uint16_t ID){
    for(int i = 0; i < IgnSz; ++i)
        if(Ignore_IDs[i] == ID) return 0;
    return 1;
}
