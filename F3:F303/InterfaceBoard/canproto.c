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

// read private defines from "canproto.h"
#define CANPRIVATE__

#include "can.h"
#include "canproto.h"
#include "hardware.h"

#define printu(x)       PRIstr(u2str(x))
#define printuhex(x)    PRIstr(uhex2str(x))

// software ignore buffer size
#define IGN_SIZE 10

extern volatile uint32_t Tms;

uint8_t CANShowMsgs = 1;
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
                PRIstrn("ID should be 11-bit number!");
                return NULL;
            }
            canmsg.ID = (uint16_t)(N&0x7ff);
            ctr = 0;
            continue;
        }
        if(ctr > 7){
            PRIstrn("ONLY 8 data bytes allowed!");
            return NULL;
        }
        if(N > 0xff){
            PRIstrn("Every data portion is a byte!");
            return NULL;
        }
        canmsg.data[ctr++] = (uint8_t)(N&0xff);
    }while(1);
    if(canmsg.ID == 0xffff){
        PRIstrn("NO ID given, send nothing!");
        return NULL;
    }
    PRIstrn("Message parsed OK");
    canmsg.length = (uint8_t) ctr;
    return &canmsg;
}

// Send command, format: ID (hex/bin/dec) data bytes (up to 8 bytes, space-delimeted)
TRUE_INLINE void CANcommand(char *txt){
    if(CAN->MSR & CAN_MSR_INAK){
        PRIstrn("CAN bus is off, try to restart it");
        return;
    }
    CAN_message *msg = parseCANmsg(txt);
    if(!msg) return;
    uint32_t N = 5;
    while(CAN_BUSY == CAN_send(msg->data, msg->length, msg->ID)){
        if(--N == 0) break;
    }
}

TRUE_INLINE void CANini(const char *txt){
    uint32_t N;
    const char *n = getnum(txt, &N);
    if(txt == n){
        PRIstr("CANspeed=");
        PRIstrn(u2str(CAN_speed()));
        return;
    }
    if(N < CAN_MIN_SPEED){
        PRIstrn("Speed is too low");
        return;
    }else if(N > CAN_MAX_SPEED){
        PRIstrn("Speed is too large");
        return;
    }
    CAN_reinit(N);
    PRIstr("Reinit CAN bus with speed ");
    printu(N); PRIn();
}

TRUE_INLINE void addIGN(const char *txt){
    if(IgnSz == IGN_SIZE){
        PRIstrn("Ignore buffer is full");
        return;
    }
    txt = omit_spaces(txt);
    uint32_t N;
    const char *n = getnum(txt, &N);
    if(txt == n){
        PRIstrn("No ID given");
        return;
    }
    if(N > 0x7ff){
        PRIstrn("ID should be 11-bit number!");
        return;
    }
    Ignore_IDs[IgnSz++] = (uint16_t)(N & 0x7ff);
    PRIstr("Added ID "); printu(N);
    PRIstr("\nIgn buffer size: "); PRIstrn(u2str(IgnSz));
}

TRUE_INLINE void print_ign_buf(){
    if(IgnSz == 0){
        PRIstrn("Ignore buffer is empty");
        return;
    }
    PRIstr("Ignored IDs:");
    for(int i = 0; i < IgnSz; ++i){
        printu(i);
        PRIstr(": ");
        PRIstrn(u2str(Ignore_IDs[i]));
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
            PRIstr("Filter "); printu(ctr); PRIstr(", FIFO");
            if(CAN->FFA1R & mask) PRIstr("1");
            else PRIstr("0");
            PRIstr(" in ");
            if(CAN->FM1R & mask){ // up to 4 filters in LIST mode
                PRIstr("LIST mode, IDs: ");
                printID(CAN->sFilterRegister[ctr].FR1 & 0xffff);
                PRIstr(" ");
                printID(CAN->sFilterRegister[ctr].FR1 >> 16);
                PRIstr(" ");
                printID(CAN->sFilterRegister[ctr].FR2 & 0xffff);
                PRIstr(" ");
                printID(CAN->sFilterRegister[ctr].FR2 >> 16);
            }else{ // up to 2 filters in MASK mode
                PRIstr("MASK mode: ");
                if(!(CAN->sFilterRegister[ctr].FR1&0x1f)){
                    PRIstr("ID="); printID(CAN->sFilterRegister[ctr].FR1 & 0xffff);
                    PRIstr(", MASK="); printID(CAN->sFilterRegister[ctr].FR1 >> 16);
                    PRIstr(" ");
                }
                if(!(CAN->sFilterRegister[ctr].FR2&0x1f)){
                    PRIstr("ID="); printID(CAN->sFilterRegister[ctr].FR2 & 0xffff);
                    PRIstr(", MASK="); printID(CAN->sFilterRegister[ctr].FR2 >> 16);
                }
            }
            PRIn();
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
    if(s != n) floodT = N;
    PRIstr("t="); PRIstrn(u2str(floodT));
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
        PRIstrn("No bank# given");
        return;
    }
    if(N > STM32FBANKNO-1){
        PRIstrn("bank# too big");
        return;
    }
    uint8_t bankno = (uint8_t)N;
    str = omit_spaces(n);
    if(!*str){ // deactivate filter
        PRIstr("Deactivate filters in bank ");
        printu(bankno);
        PRIn();
        CAN->FMR = CAN_FMR_FINIT;
        CAN->FA1R &= ~(1<<bankno);
        CAN->FMR &=~ CAN_FMR_FINIT;
        return;
    }
    uint8_t fifono = 0;
    if(*str == '1') fifono = 1;
    else if(*str != '0'){
        PRIstrn("FIFO# is 0 or 1");
        return;
    }
    str = omit_spaces(str + 1);
    const char c = *str;
    uint8_t mode = 0; // ID
    if(c == 'M' || c == 'm') mode = 1;
    else if(c != 'I' && c != 'i'){
        PRIstrn("mode is 'M/m' for MASK and 'I/i' for IDLIST");
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
        PRIstrn("You should add at least one filter!");
        return;
    }
    if(mode && (nfilt&1)){
        PRIstrn("In MASK mode you should point pairs of ID/MASK");
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
    PRIstr("Added filter with ");
    printu(nfilt); PRIstrn(" parameters");
}

static const char *helpstring =
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
    "'p' - print ignore buffer\n"
    "'P' - pause/resume in packets displaying\n"
    "'R' - software reset\n"
    "'s/S' - send data over CAN: s ID byte0 .. byteN\n"
    "'t' - change flood period (>=0ms)\n"
    "'T' - get time from start (ms)\n"
;


TRUE_INLINE void getcanstat(){
    PRIstr("CAN_MSR=");
    printuhex(CAN->MSR);
    PRIstr("\nCAN_TSR=");
    printuhex(CAN->TSR);
    PRIstr("\nCAN_RF0R=");
    printuhex(CAN->RF0R);
    PRIstr("\nCAN_RF1R=");
    PRIstrn(u2str(CAN->RF1R));
}

/**
 * @brief CANcmd_parser - CAN command parsing
 * @param txt   - buffer with commands & data
 * @param isUSB - == 1 if data got from USB
 */
void CANcmd_parser(char *txt){
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
            CAN_flood(parseCANmsg(txt), 0);
            return;
        break;
        case 's':
        case 'S':
            CANcommand(txt);
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
            CAN_printerr();
        break;
        case 'i':
            CAN_flood(NULL, 1);
            PRIstrn("Incremental flooding is ON ('F' to off)");
        break;
        case 'I':
            CAN_reinit(0);
            PRIstr("CANspeed=");
            PRIstrn(u2str(CAN_speed()));
        break;
        case 'l':
            list_filters();
        break;
        case 'p':
            print_ign_buf();
        break;
        case 'P':
            CANShowMsgs = !CANShowMsgs;
            if(CANShowMsgs) PRIstrn("Resume");
            else PRIstrn("Pause");
        break;
        case 'R':
            PRIstrn("Soft reset");
            USB_sendall(ICAN); // wait until everything will gone
            NVIC_SystemReset();
        break;
        case 'T':
            PRIstr("Time (ms): ");
            PRIstrn(u2str(Tms));
        break;
        default: // help
            PRIstrn(helpstring);
        break;
    }
}

// check Ignore_IDs & return 1 if ID isn't in list
uint8_t CANsoftFilter(uint16_t ID){
    for(int i = 0; i < IgnSz; ++i)
        if(Ignore_IDs[i] == ID) return 0;
    return 1;
}

// process incoming USB and CAN messages
void canproto_process(){
    CAN_proc();
    if(CAN_get_status() == CAN_FIFO_OVERRUN){
        USB_sendstr(ICAN, "CAN bus fifo overrun occured!\n");
    }
    CAN_message *can_mesg;
    while((can_mesg = CAN_messagebuf_pop())){
        if(can_mesg && CANsoftFilter(can_mesg->ID)){
            if(CANShowMsgs){ // display message content
                IWDG->KR = IWDG_REFRESH;
                uint8_t len = can_mesg->length;
                USB_sendstr(ICAN, u2str(Tms));
                USB_sendstr(ICAN, " #");
                USB_sendstr(ICAN, uhex2str(can_mesg->ID));
                for(uint8_t i = 0; i < len; ++i){
                    USB_putbyte(ICAN, ' ');
                    USB_sendstr(ICAN, uhex2str(can_mesg->data[i]));
                }
                newline(ICAN);
            }
        }
    }
}
