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

#include "adc.h"
#include "buttons.h"
#include "can.h"
#include "commonproto.h"
#include "flash.h"
#include "hardware.h"
#include "hdr.h"
#include "proto.h"
#include "steppers.h"
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

int fn_canignore(uint32_t _U_ hash,  char *args){
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

int fn_canreinit(uint32_t _U_ hash, char _U_ *args){
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

int fn_canfilter(uint32_t _U_ hash,  char *args){
    if(*args) add_filter(args);
    else list_filters();
    return RET_GOOD;
}

int fn_canflood(uint32_t _U_ hash,  char *args){
    CAN_flood(parseCANmsg(args), 0);
    return RET_GOOD;
}

int fn_cansend(uint32_t _U_ hash, char *args){
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

int fn_canfloodt(uint32_t _U_ hash, char *args){
    uint32_t N;
    const char *n = getnum(args, &N);
    if(args != n) floodT = N;
    USB_sendstr("canfloodT="); printu(floodT); USB_putbyte('\n');
    return RET_GOOD;
}

int fn_canstat(uint32_t _U_ hash,  char _U_ *args){
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

int fn_canerrcodes(uint32_t _U_ hash,  char _U_ *args){
    CAN_printerr();
    return RET_GOOD;
}

int fn_canincrflood(uint32_t _U_ hash,  char _U_ *args){
    CAN_flood(NULL, 1);
    USB_sendstr("Incremental flooding is ON ('F' to off)\n");
    return RET_GOOD;
}

int fn_canspeed(uint32_t _U_ hash,  char _U_ *args){
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

int fn_canpause(uint32_t _U_ hash,  char _U_ *args){
    ShowMsgs = TRUE;
    USND("Pause CAN messages");
    return RET_GOOD;
}

int fn_canresume(uint32_t _U_ hash,  char _U_ *args){
    ShowMsgs = FALSE;
    USND("RESUME CAN messages");
    return RET_GOOD;
}

// dump base commands codes (for CAN protocol)
int fn_dumpcmd(uint32_t _U_ hash,  char _U_ *args){ // "dumpcmd" (1223955823)
    USB_sendstr("CANbus commands list:\n");
    for(uint16_t i = 1; i < CCMD_AMOUNT; ++i){
        if(!cancmds[i]) continue;
        printu(i);
        USB_sendstr(" - ");
        USB_sendstr(cancmds[i]);
        newline();
    }
    return RET_GOOD;
}

int fn_reset(uint32_t _U_ hash,  char _U_ *args){ // "reset" (1907803304)
    USB_sendstr("Soft reset\n");
    USB_sendall(); // wait until everything will gone
    NVIC_SystemReset();
    return RET_GOOD;
}

static const char* motfl[MOTFLAGS_AMOUNT] = {
    "0: reverse - invert motor's rotation",
    "1: [reserved]",
    "2: [reserved]",
    "3: donthold - clear motor's power after stop",
    "4: eswinv - inverse end-switches (1->0 instead of 0->1)",
    "5: [reserved]",
    "6,7: drvtype - driver type (0 - only step/dir, 1 - UART, 2 - SPI, 3 - reserved)"
};
static const char *eswfl[ESW_AMOUNT] = {
    [ESW_IGNORE] = "ignore end-switches",
    [ESW_ANYSTOP] = "stop @ esw in any moving direction",
    [ESW_STOPMINUS] = "stop only when moving in given direction (e.g. to minus @ESW0)"
};
int fn_dumpmotflags(uint32_t _U_ hash,  char _U_ *args){ // "dumpmotflags" (36159640)
    USB_sendstr("Motor flags:");
    for(int i = 0; i < MOTFLAGS_AMOUNT; ++i){
        USB_sendstr("\nbit"); printu(i); USB_sendstr(" - ");
        USB_sendstr(motfl[i]);
    }
    USND("\nEnd-switches reaction:");
    for(int i = 0; i < ESW_AMOUNT; ++i){
        printu(i); USB_sendstr(" - ");
        USB_sendstr(eswfl[i]); newline();
    }
    return RET_GOOD;
}

static const char* errtxt[ERR_AMOUNT] = {
    [ERR_OK] =   "OK",
    [ERR_BADPAR] =  "BADPAR",
    [ERR_BADVAL] = "BADVAL",
    [ERR_WRONGLEN] = "WRONGLEN",
    [ERR_BADCMD] = "BADCMD",
    [ERR_CANTRUN] = "CANTRUN",
};
int fn_dumperr(uint32_t _U_ hash,  char _U_ *args){ // "dumperr" (1223989764)
    USND("Error codes:");
    for(int i = 0; i < ERR_AMOUNT; ++i){
        printu(i); USB_sendstr(" - ");
        USB_sendstr(errtxt[i]); newline();
    }
    return RET_GOOD;
}

static const char* motstates[STP_STATE_AMOUNT] = {
    [STP_RELAX] = "relax",
    [STP_ACCEL] = "acceleration",
    [STP_MOVE] = "moving",
    [STP_MVSLOW] = "moving at lowest speed",
    [STP_DECEL] = "deceleration",
    [STP_STALL] = "stalled (not used here!)",
    [STP_ERR] = "error"
};
int fn_dumpstates(uint32_t _U_ hash,  char _U_ *args){ // "dumpstates" (4235564367)
    USND("Motor's state codes:");
    for(int i = 0; i < STP_STATE_AMOUNT; ++i){
        printu(i); USB_sendstr(" - ");
        USB_sendstr(motstates[i]); newline();
    }
    return RET_GOOD;
}

int fn_canid(uint32_t _U_ hash, char *args){ // "canid" (2040257924)
    if(args && *args){
        int good = FALSE;
        uint32_t N;
        const char *eq = getnum(args, &N);
        if(eq != args && N < 0xfff){
            the_conf.CANID = (uint16_t)N;
            CAN_reinit(the_conf.CANspeed);
            good = TRUE;
        }
        if(!good) USB_sendstr("CANID setter format: `canid=ID`, ID is 11bit\n");
    }
    USB_sendstr("canid="); USB_sendstr(uhex2str(the_conf.CANID));
    newline();
    return RET_GOOD;
}


static int canusb_function(uint32_t hash, char *args){
    errcodes e = ERR_BADCMD;
    uint32_t N;
    int32_t val = 0;
    uint8_t par = CANMESG_NOPAR;
    DBG("CMD: hash=");
#ifdef EBUG
    printu(hash); USB_sendstr(", args=");
    USND(args);
#endif
    if(*args){
        const char *n = getnum(args, &N);
        if(n != args){ // get parameter
            if(N >= CANMESG_NOPAR){
                USND("Wrong parameter value");
                return RET_GOOD;
            }
            par = (uint8_t) N;
        }
        n = strchr(n, '=');
        if(n){
            ++n;
            const char *nxt = getint(n, &val);
            if(nxt != n){ // set setter flag
                par |= SETTERFLAG;
            }
        }
    }
#ifdef EBUG
    USB_sendstr("par="); printuhex(par);
    USB_sendstr(", val="); printi(val); newline();
#endif
    switch(hash){
        case CMD_ADC:
            e = cu_adc(par, &val);
        break;
        case CMD_BUTTON:
            e = cu_button(par, &val);
            if(val == CANMESG_NOPAR) break; // no button number
            const char *kstate = "none";
            switch(e){
                case EVT_PRESS:
                    kstate = "press";
                break;
                case EVT_HOLD:
                    kstate = "hold";
                break;
                case EVT_RELEASE:
                    kstate = "release";
                break;
                default:
                break;
            }
            USB_sendstr("button"); USB_putbyte('0'+PARBASE(par));
            USB_putbyte('='); USB_sendstr(kstate);
            USB_sendstr("\nbuttontm="); USB_sendstr(u2str(val));
            newline();
            return RET_GOOD;
        break;
        case CMD_ESW:
            e = cu_esw(par, &val);
        break;
        case CMD_ERASEFLASH:
            e = cu_eraseflash(par, &val);
        break;
        case CMD_ACCEL:
            e = cu_accel(par, &val);
        break;
        case CMD_ABSPOS:
            e = cu_abspos(par, &val);
        break;
        case CMD_DIAGN:
            e = cu_diagn(par, &val);
        break;
        case CMD_DRVTYPE:
            e = cu_drvtype(par, &val);
        break;
        case CMD_EMSTOP:
            e = cu_emstop(par, &val);
        break;
        case CMD_ESWREACT:
            e = cu_eswreact(par, &val);
        break;
        case CMD_GOTO:
            e = cu_goto(par, &val);
        break;
        case CMD_GOTOZ:
            e = cu_gotoz(par, &val);
        break;
        case CMD_GPIO:
            e = cu_gpio(par, &val);
        break;
        case CMD_GPIOCONF:
            e = cu_gpioconf(par, &val);
        break;
        case CMD_MAXSPEED:
            e = cu_maxspeed(par, &val);
        break;
        case CMD_MAXSTEPS:
            e = cu_maxsteps(par, &val);
        break;
        case CMD_MCUT:
            e = cu_mcut(par, &val);
        break;
        case CMD_MCUVDD:
            e = cu_mcuvdd(par, &val);
        break;
        case CMD_MICROSTEPS:
            e = cu_microsteps(par, &val);
        break;
        case CMD_MINSPEED:
            e = cu_minspeed(par, &val);
        break;
        case CMD_MOTCURRENT:
            e = cu_motcurrent(par, &val);
        break;
        case CMD_MOTFLAGS:
            e = cu_motflags(par, &val);
        break;
        case CMD_MOTMUL:
            e = cu_motmul(par, &val);
        break;
        case CMD_MOTNO:
            e = cu_motno(par, &val);
        break;
        case CMD_MOTREINIT:
            e = cu_motreinit(par, &val);
        break;
        case CMD_PDN:
            e = cu_pdn(par, &val);
        break;
        case CMD_PING:
            e = cu_ping(par, &val);
        break;
        case CMD_RELPOS:
            e = cu_relpos(par, &val);
        break;
        case CMD_RELSLOW:
            e = cu_relslow(par, &val);
        break;
        case CMD_SAVECONF:
            e = cu_saveconf(par, &val);
        break;
        case CMD_SCREEN:
            e = cu_screen(par, &val);
        break;
        case CMD_SPEEDLIMIT:
            e = cu_speedlimit(par, &val);
        break;
        case CMD_STATE:
            e = cu_state(par, &val);
        break;
        case CMD_STOP:
            e = cu_stop(par, &val);
        break;
        case CMD_TIME:
            e = cu_time(par, &val);
        break;
        case CMD_TMCBUS:
            e = cu_tmcbus(par, &val);
        break;
        case CMD_UDATA:
            e = cu_udata(par, &val);
        break;
        case CMD_USARTSTATUS:
            e = cu_usartstatus(par, &val);
        break;
        case CMD_VDRIVE:
            e = cu_vdrive(par, &val);
        break;
        case CMD_VFIVE:
            e = cu_vfive(par, &val);
        break;
        default:
            e = ERR_BADCMD;
            break;
    }

    //if(e < ERR_OK || e >= ERR_AMOUNT) USND("Bad return code");
    //else
    if(ERR_OK != e){
        USB_sendstr(errtxt[e]); newline();
    }else{
        USB_sendstr(lastcmd);
        if(PARBASE(par) != CANMESG_NOPAR) printu(PARBASE(par));
        USB_putbyte('='); printi(val);
        newline();
    }
    return RET_GOOD;
}

#define AL __attribute__ ((alias ("canusb_function")))

// COMMON with CAN
int fn_abspos(uint32_t _U_ hash,  char _U_ *args) AL; //* "abspos" (3056382221)
int fn_accel(uint32_t _U_ hash,  char _U_ *args) AL; //* "accel" (1490521981)
int fn_adc(uint32_t _U_ hash,  char _U_ *args) AL; // "adc" (2963026093)
int fn_button(uint32_t _U_ hash,  char _U_ *args) AL; // "button" (1093508897)
int fn_diagn(uint32_t _U_ hash,  char _U_ *args) AL; //* "diagn" (2334137736)
int fn_drvtype(uint32_t _U_ hash, char _U_ *args) AL; // "drvtype" (3930242451)
int fn_emstop(uint32_t _U_ hash,  char _U_ *args) AL; //* "emstop" (2965919005)
int fn_eraseflash(uint32_t _U_ hash,  char _U_ *args) AL; //* "eraseflash" (3177247267)
int fn_esw(uint32_t _U_ hash,  char _U_ *args) AL; // "esw" (2963094612)
int fn_eswreact(uint32_t _U_ hash,  char _U_ *args) AL; //* "eswreact" (1614224995)
int fn_goto(uint32_t _U_ hash,  char _U_ *args) AL; //* "goto" (4286309438)
int fn_gotoz(uint32_t _U_ hash,  char _U_ *args) AL; //* "gotoz" (3178103736)
int fn_gpio(uint32_t _U_ hash,  char _U_ *args) AL; //* "gpio" (4286324660)
int fn_gpioconf(uint32_t _U_ hash,  char _U_ *args) AL; //* "gpioconf" (1309721562)
int fn_maxspeed(uint32_t _U_ hash,  char _U_ *args) AL; //* "maxspeed" (1498078812)
int fn_maxsteps(uint32_t _U_ hash,  char _U_ *args) AL; //* "maxsteps" (1506667002)
int fn_mcut(uint32_t _U_ hash,  char _U_ *args) AL; // "mcut" (4022718)
int fn_mcuvdd(uint32_t _U_ hash,  char _U_ *args) AL; // "mcuvdd" (2517587080)
int fn_microsteps(uint32_t _U_ hash,  char _U_ *args) AL; //* "microsteps" (3974395854)
int fn_minspeed(uint32_t _U_ hash,  char _U_ *args) AL; //* "minspeed" (3234848090)
int fn_motcurrent(uint32_t _U_ hash, char _U_ *args) AL; // "motcurrent" (1926997848)
int fn_motflags(uint32_t _U_ hash,  char _U_ *args) AL; //* "motflags" (2153634658)
int fn_motmul(uint32_t _U_ hash,  char _U_ *args) AL; //* "motmul" (1543400099)
int fn_motno(uint32_t _U_ hash, char _U_ *args) AL; // "motno" (544673586)
int fn_motreinit(uint32_t _U_ hash,  char _U_ *args) AL; //* "motreinit" (199682784)
int fn_pdn(uint32_t _U_ hash, char _U_ *args) AL; // "pdn" (2963275719)
int fn_ping(uint32_t _U_ hash,  char _U_ *args) AL; // "ping" (10561715)
int fn_relpos(uint32_t _U_ hash,  char _U_ *args) AL; //* "relpos" (1278646042)
int fn_relslow(uint32_t _U_ hash,  char _U_ *args) AL; //* "relslow" (1742971917)
int fn_saveconf(uint32_t _U_ hash,  char _U_ *args) AL; //* "saveconf" (141102426)
int fn_screen(uint32_t _U_ hash,  char _U_ *args) AL; //* "screen" (2100809349)
int fn_speedlimit(uint32_t _U_ hash,  char _U_ *args) AL; //* "speedlimit" (1654184245)
int fn_state(uint32_t _U_ hash,  char _U_ *args) AL; //* "state" (2216628902)
int fn_stop(uint32_t _U_ hash,  char _U_ *args) AL; //* "stop" (17184971)
int fn_time(uint32_t _U_ hash,  char _U_ *args) AL; // "time" (19148340)
int fn_tmcbus(uint32_t _U_ hash,  char _U_ *args) AL; //* "tmcbus" (1906135955)
int fn_udata(uint32_t _U_ hash,  char _U_ *args) AL; //* "udata" (2736127636)
int fn_usartstatus(uint32_t _U_ hash,  char _U_ *args) AL; //* "usartstatus" (4007098968)
int fn_vdrive(uint32_t _U_ hash, char _U_ *args) AL; // "vdrive" (2172773525)
int fn_vfive(uint32_t _U_ hash, char _U_ *args) AL; // "vfive" (3017477285)


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
