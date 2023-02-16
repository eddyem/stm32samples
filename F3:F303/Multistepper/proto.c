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
#include "flash.h"
#include "hardware.h"
#include "hdr.h"
#include "proto.h"
#include "commonproto.h"
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

// dump base commands codes (for CAN protocol)
int fn_dumpcmd(_U_ uint32_t hash,  _U_ char *args){ // "dumpcmd" (1223955823)
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

int fn_reset(_U_ uint32_t hash,  _U_ char *args){ // "reset" (1907803304)
    USB_sendstr("Soft reset\n");
    USB_sendall(); // wait until everything will gone
    NVIC_SystemReset();
    return RET_GOOD;
}

int fn_time(_U_ uint32_t hash,  _U_ char *args){ // "time" (19148340)
    USB_sendstr("Time (ms): ");
    printu(Tms);
    USB_putbyte('\n');
    return RET_GOOD;
}

static const char* errtxt[ERR_AMOUNT] = {
    [ERR_OK] =   "all OK",
    [ERR_BADPAR] =  "wrong parameter's value",
    [ERR_BADVAL] = "wrong setter of parameter",
    [ERR_WRONGLEN] = "bad message length",
    [ERR_BADCMD] = "unknown command",
    [ERR_CANTRUN] = "temporary can't run given command",
};
int fn_dumperr(_U_ uint32_t hash,  _U_ char *args){ // "dumperr" (1223989764)
    USND("Error codes:");
    for(int i = 0; i < ERR_AMOUNT; ++i){
        printu(i); USB_sendstr(" - ");
        USB_sendstr(errtxt[i]); newline();
    }
    return RET_GOOD;
}

int fn_canid(_U_ uint32_t hash, char *args){ // "canid" (2040257924)
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
    float f;
    if(*args){
        const char *n = getnum(args, &N);
        if(n != args){ // get parameter
            if(N >= CANMESG_NOPAR){
                USND("Wrong parameter value");
                return RET_GOOD;
            }
            par = (uint8_t) N;
            n = strchr(n, '=');
            if(n){
                const char *nxt = getnum(n, &N);
                if(nxt != n){ // give flag issetter
                    val = (int32_t) N;
                    par |= SETTERFLAG;
                }
            }
        }
    }
    switch(hash){
        case CMD_PING:
            e = cu_ping(par, &val);
        break;
        case CMD_MCUT:
            f = getMCUtemp();
            USB_sendstr("T=");
            USB_sendstr(float2str(f, 1));
            newline();
            return RET_GOOD;
        break;
        case CMD_MCUVDD:
            f = getVdd();
            USB_sendstr("VDD=");
            USB_sendstr(float2str(f, 1));
            newline();
            return RET_GOOD;
        break;
        case CMD_ADC:
            par = PARBASE(par);
            if(par >= NUMBER_OF_ADC_CHANNELS){
                USB_sendstr("Wrong channel number\n");
                return RET_BAD;
            }
            USB_sendstr("ADC"); USB_putbyte('0'+par);
            USB_putbyte('='); USB_sendstr(u2str(getADCval(par)));
            f = getADCvoltage(par);
            USB_sendstr("\nADCv");USB_putbyte('0'+par);
            USB_putbyte('='); USB_sendstr(float2str(f, 1));
            newline();
            return RET_GOOD;
        break;
        case CMD_BUTTON:
            e = cu_button(par, &val);
            if(val == CANMESG_NOPAR){
                USB_sendstr("Wrong button number\n");
                return RET_BAD;
            }
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
            USB_sendstr("KEY"); USB_putbyte('0'+PARBASE(par));
            USB_putbyte('='); USB_sendstr(kstate);
            USB_sendstr("KEYTIME="); USB_sendstr(u2str(val));
            newline();
            return RET_GOOD;
        break;
        case CMD_ESW:
            e = cu_esw(par, &val);
        break;
        default:
            break;
    }

    //if(e < ERR_OK || e >= ERR_AMOUNT) USND("Bad return code");
    //else
    if(ERR_OK != e){
        USB_sendstr(errtxt[e]); newline();
    }else{
        USB_sendstr("OK par");
        if(par != CANMESG_NOPAR) printu(PARBASE(par));
        USB_putbyte('='); printi(val);
        newline();
    }
    return RET_GOOD;
}

#define AL __attribute__ ((alias ("canusb_function")))

// COMMON with CAN
int fn_ping(_U_ uint32_t hash,  _U_ char *args) AL; // "ping" (10561715)
// not realized yet
int fn_abspos(_U_ uint32_t hash,  _U_ char *args) AL; //* "abspos" (3056382221)
int fn_accel(_U_ uint32_t hash,  _U_ char *args) AL; //* "accel" (1490521981)
int fn_adc(_U_ uint32_t hash,  _U_ char *args) AL; // "adc" (2963026093)
int fn_button(_U_ uint32_t hash,  _U_ char *args) AL; // "button" (1093508897)
int fn_diagn(_U_ uint32_t hash,  _U_ char *args) AL; //* "diagn" (2334137736)
int fn_emstop(_U_ uint32_t hash,  _U_ char *args) AL; //* "emstop" (2965919005)
int fn_eraseflash(_U_ uint32_t hash,  _U_ char *args) AL; //* "eraseflash" (3177247267)
int fn_esw(_U_ uint32_t hash,  _U_ char *args) AL; // "esw" (2963094612)
int fn_eswreact(_U_ uint32_t hash,  _U_ char *args) AL; //* "eswreact" (1614224995)
int fn_goto(_U_ uint32_t hash,  _U_ char *args) AL; //* "goto" (4286309438)
int fn_gotoz(_U_ uint32_t hash,  _U_ char *args) AL; //* "gotoz" (3178103736)
int fn_gpio(_U_ uint32_t hash,  _U_ char *args) AL; //* "gpio" (4286324660)
int fn_gpioconf(_U_ uint32_t hash,  _U_ char *args) AL; //* "gpioconf" (1309721562)
int fn_maxspeed(_U_ uint32_t hash,  _U_ char *args) AL; //* "maxspeed" (1498078812)
int fn_maxsteps(_U_ uint32_t hash,  _U_ char *args) AL; //* "maxsteps" (1506667002)
int fn_mcut(_U_ uint32_t hash,  _U_ char *args) AL; // "mcut" (4022718)
int fn_mcuvdd(_U_ uint32_t hash,  _U_ char *args) AL; // "mcuvdd" (2517587080)
int fn_microsteps(_U_ uint32_t hash,  _U_ char *args) AL; //* "microsteps" (3974395854)
int fn_minspeed(_U_ uint32_t hash,  _U_ char *args) AL; //* "minspeed" (3234848090)
int fn_motflags(_U_ uint32_t hash,  _U_ char *args) AL; //* "motflags" (2153634658)
int fn_motmul(_U_ uint32_t hash,  _U_ char *args) AL; //* "motmul" (1543400099)
int fn_motreinit(_U_ uint32_t hash,  _U_ char *args) AL; //* "motreinit" (199682784)
int fn_relpos(_U_ uint32_t hash,  _U_ char *args) AL; //* "relpos" (1278646042)
int fn_relslow(_U_ uint32_t hash,  _U_ char *args) AL; //* "relslow" (1742971917)
int fn_saveconf(_U_ uint32_t hash,  _U_ char *args) AL; //* "saveconf" (141102426)
int fn_screen(_U_ uint32_t hash,  _U_ char *args) AL; //* "screen" (2100809349)
int fn_speedlimit(_U_ uint32_t hash,  _U_ char *args) AL; //* "speedlimit" (1654184245)
int fn_state(_U_ uint32_t hash,  _U_ char *args) AL; //* "state" (2216628902)
int fn_stop(_U_ uint32_t hash,  _U_ char *args) AL; //* "stop" (17184971)
int fn_tmcbus(_U_ uint32_t hash,  _U_ char *args) AL; //* "tmcbus" (1906135955)
int fn_udata(_U_ uint32_t hash,  _U_ char *args) AL; //* "udata" (2736127636)
int fn_usartstatus(_U_ uint32_t hash,  _U_ char *args) AL; //* "usartstatus" (4007098968)


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
