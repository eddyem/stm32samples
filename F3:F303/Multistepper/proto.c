/*
 * This file is part of the multistepper project.
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
#include "hdr.h"
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

// print ID/mask of CAN->sFilterRegister[x] half
static void printID(uint16_t FRn){
    if(FRn & 0x1f) return; // trash
    printuhex(FRn >> 5);
}

const char *helpstring =
    "https://github.com/eddyem/stm32samples/tree/master/F3:F303/Multistepper build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
    "Format: cmd [N] - getter, cmd [N] = val - setter; N - optional index\n"
    "* - command not supported yet, G - getter, S - setter\n\n"
#include "hashgen/helpcmds.in"
;

int fn_canignore(_U_ uint32_t hash,  char *args){
    if(!*args){
        if(IgnSz == 0){
            USND("Ignore buffer is empty");
            return RET_GOOD;
        }
        USND("Ignored IDs:");
        for(int i = 0; i < IgnSz; ++i){
            printu(i);
            USB_sendstr(": ");
            printuhex(Ignore_IDs[i]);
            USB_putbyte('\n');
        }
        return RET_GOOD;
    }
    if(IgnSz == IGN_SIZE){
        USND("Ignore buffer is full");
        return RET_GOOD;
    }
    int32_t N;
    const char *n = getint(args, &N);
    if(args == n){
        USND("No ID given");
        return RET_GOOD;
    }
    if(N < 0){
         IgnSz = 0;
         USND("Ignore list cleared");
         return RET_GOOD;
    }
    if(N > 0x7ff){
        USND("ID should be 11-bit number!");
        return RET_GOOD;
    }
    Ignore_IDs[IgnSz++] = (uint16_t)(N & 0x7ff);
    USB_sendstr("Added ID "); printu(N);
    USB_sendstr("\nIgn buffer size: "); printu(IgnSz);
    newline();
    return RET_GOOD;
}

int fn_canreinit(_U_ uint32_t hash, _U_ char *args){
    CAN_reinit(0);
    USND("OK");
    return RET_GOOD;
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

int fn_canfilter(_U_ uint32_t hash,  char *args){
    if(*args) add_filter(args);
    else list_filters();
    return RET_GOOD;
}

int fn_canflood(_U_ uint32_t hash,  char *args){
    CAN_flood(parseCANmsg(args), 0);
    return RET_GOOD;
}

int fn_cansend(_U_ uint32_t hash, char *args){
    if(CAN->MSR & CAN_MSR_INAK){
        USB_sendstr("CAN bus is off, try to restart it\n");
        return RET_GOOD;
    }
    CAN_message *msg = parseCANmsg(args);
    if(!msg) return RET_WRONGCMD;
    uint32_t N = 5;
    while(CAN_BUSY == CAN_send(msg->data, msg->length, msg->ID)){
        if(--N == 0) break;
    }
    if(N == 0) return RET_BAD;
    return RET_GOOD;
}

int fn_canfloodt(_U_ uint32_t hash, char *args){
    uint32_t N;
    const char *n = getnum(args, &N);
    if(args == n){
        USB_sendstr("floodT="); printu(floodT); USB_putbyte('\n');
        return RET_GOOD;
    }
    floodT = N;
    return RET_GOOD;
}

int fn_canstat(_U_ uint32_t hash,  _U_ char *args){
    USB_sendstr("CAN_MSR=");
    printuhex(CAN->MSR);
    USB_sendstr("\nCAN_TSR=");
    printuhex(CAN->TSR);
    USB_sendstr("\nCAN_RF0R=");
    printuhex(CAN->RF0R);
    USB_sendstr("\nCAN_RF1R=");
    printuhex(CAN->RF1R);
    newline();
    return RET_GOOD;
}

int fn_canerrcodes(_U_ uint32_t hash,  _U_ char *args){
    CAN_printerr();
    return RET_GOOD;
}

int fn_canincrflood(_U_ uint32_t hash,  _U_ char *args){
    CAN_flood(NULL, 1);
    USB_sendstr("Incremental flooding is ON ('F' to off)\n");
    return RET_GOOD;
}

int fn_canspeed(_U_ uint32_t hash,  _U_ char *args){
    uint32_t N;
    const char *n = getnum(args, &N);
    if(args == n){
        USB_sendstr("No speed given");
        return RET_GOOD;
    }
    if(N < 50){
        USB_sendstr("canspeed=");
        USB_sendstr(u2str(CAN_speed()));
        newline();
        return RET_GOOD;
    }else if(N > 3000){
        USB_sendstr("Highest speed is 3000kbps");
        return RET_GOOD;
    }
    CAN_reinit((uint16_t)N);
    // theconf.canspeed = N;
    USB_sendstr("Reinit CAN bus with speed ");
    printu(N); USB_sendstr("kbps"); newline();
    return RET_GOOD;
}

int fn_canpause(_U_ uint32_t hash,  _U_ char *args){
    ShowMsgs = TRUE;
    USND("Pause CAN messages");
    return RET_GOOD;
}

int fn_canresume(_U_ uint32_t hash,  _U_ char *args){
    ShowMsgs = FALSE;
    USND("RESUME CAN messages");
    return RET_GOOD;
}

int fn_reset(_U_ uint32_t hash,  _U_ char *args){
    USB_sendstr("Soft reset\n");
    USB_sendall(); // wait until everything will gone
    NVIC_SystemReset();
    return RET_GOOD;
}

int fn_time(_U_ uint32_t hash,  _U_ char *args){
    USB_sendstr("Time (ms): ");
    printu(Tms);
    USB_putbyte('\n');
    return RET_GOOD;
}

/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 */
const char *cmd_parser(const char *txt){
    int ret = parsecmd(txt);
    switch(ret){
        case RET_CMDNOTFOUND: return helpstring; break;
        case RET_WRONGCMD: return "Wrong command parameters\n"; break;
        case RET_GOOD: return NULL; break;
        default: return "FAIL\n"; break;
    }
}

// check Ignore_IDs & return 1 if ID isn't in list
uint8_t isgood(uint16_t ID){
    for(int i = 0; i < IgnSz; ++i)
        if(Ignore_IDs[i] == ID) return 0;
    return 1;
}
