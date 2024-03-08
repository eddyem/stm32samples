/*
 * This file is part of the canbus4bta project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <string.h>
#include <strings.h>

#include "can.h"
#include "commonfunctions.h"
#include "flash.h"
#include "proto.h"
#include "strfunc.h"
#include "textfunctions.h"
#include "usb.h"
#include "version.inc"

// maximal length of command
#define MAXCMDLEN   (16)

// text-only commans indexes (0 is prohibited)
typedef enum{
    TCMD_PROHIBITED,    // prohibited
    TCMD_WDTEST,        // test watchdog
    TCMD_DUMPCONF,      // dump configuration
    TCMD_CANSNIF,       // CAN sniffer/normal
    TCMD_CANSEND,       // send CAN message
    TCMD_AMOUNT
} text_cmd;


typedef struct{
    const char *cmd;    // command, if NULL - only display help message
    int idx;            // index in CAN cmd or text cmd list (if negative)
    const char *help;   // help message
} funcdescr;

// list of all text functions; should be sorted and can be grouped
static const funcdescr funclist[] = {
    {"adcmul", CMD_ADCMUL, "get/set ADC multipliers 0..3 (*1000)"},
    {"adcraw", CMD_ADCRAW, "get raw ADC values of channel 0..4"},
    {"adcv", CMD_ADCV,  "get ADC voltage of channel 0..3 (*100V)"},
    {"bounce", CMD_BOUNCE, "get/set bounce constant (ms)"},
    {"canid", CMD_CANID, "get/set CAN ID"},
    {"cansnif", -TCMD_CANSNIF, "get/change sniffer state (0 - normal, 1 - sniffer)"},
    {"canspeed", CMD_CANSPEED, "get/set CAN speed (bps)"},
    {"dumpconf", -TCMD_DUMPCONF, "dump current configuration"},
    {"encget", CMD_ENCGET, "read encoder data"},
    {"encreinit", CMD_ENCREINIT, "reinit encoder"},
    {"encssi", CMD_ENCISSSI, "encoder is SSI (1) or RS-422 (0)"},
    {"esw", CMD_GETESW, "anti-bounce read ESW of channel 0 or 1"},
    {"eswblk", CMD_GETESW_BLK, "blocking read ESW of channel 0 or 1"},
    {"eraseflash", CMD_ERASESTOR, "erase all flash storage"},
    {"mcutemp", CMD_MCUTEMP, "get MCU temperature (*10degrC)"},
    {"pepemul", CMD_EMULPEP, "emulate (1) / not (0) PEP"},
    {"relay", CMD_RELAY, "get/set relay state (0 - off, 1 - on)"},
    {"reset", CMD_RESET, "reset MCU"},
    {"s", -TCMD_CANSEND, "send CAN message: ID 0..8 data bytes"},
    {"saveconf", CMD_SAVECONF, "save configuration"},
    {"spiinit", CMD_SPIINIT, "init SPI2"},
    {"spisend", CMD_SPISEND, "send 4 bytes over SPI2"},
    {"time", CMD_TIME, "get/set time (1ms, 32bit)"},
    {"timestamp", CMD_TIMESTAMP, "get/set timestamp (2mks, 24bit)"},
    {"usartspeed", CMD_USARTSPEED, "get/set USART1 speed"},
    {"wdtest", -TCMD_WDTEST, "test watchdog"},
    {NULL, 0, NULL} // last record
};

/*********** START of all common functions list (for `textfunctions`) ***********/

static errcodes wdtest(const char _U_ *str){
    USB_sendstr("Wait for reboot\n");
    while(1){nop();}
    return ERR_OK;
}

// names of bit flags (ordered from LSE of[0])
static const char * const bitfields[] = {
    "encisSSI",
    NULL
};

static errcodes dumpconf(const char _U_ *str){
#ifdef EBUG
    uint32_t sz = FLASH_SIZE*1024;
    USB_sendstr("flashsize="); printu(sz); USB_putbyte('/');
    printu(FLASH_blocksize); USB_putbyte('='); printu(sz/FLASH_blocksize);
    USB_sendstr(" blocks\n");
#endif
    USB_sendstr("userconf_addr="); printuhex((uint32_t)Flash_Data);
    USB_sendstr("\nuserconf_idx="); printi(currentconfidx);
    USB_sendstr("\nuserconf_sz="); printu(the_conf.userconf_sz);
    USB_sendstr("\ncanspeed="); printu(the_conf.CANspeed);
    USB_sendstr("\ncanid="); printu(the_conf.CANID);
    for(int i = 0; i < ADC_TSENS; ++i){
        USB_sendstr("\nadcmul"); USB_putbyte('0'+i); USB_putbyte('=');
        USB_sendstr(float2str(the_conf.adcmul[i], 3));
    }
    USB_sendstr("\nusartspeed="); printu(the_conf.usartspeed);
    const char * const *p = bitfields;
    int bit = 0;
    while(p){
        newline();
        USB_sendstr(*p); USB_putbyte('='); USB_putbyte((the_conf.flags & (1<<bit)) ? '1' : '0');
        if(++bit > 31) break;
        ++p;
    }
//#define PROPNAME(nm)    do{newline(); USB_sendstr(nm); USB_putbyte(cur); USB_putbyte('=');}while(0)
//#undef PROPNAME
    newline();
    return ERR_OK;
}

static errcodes cansnif(const char *str){
    uint32_t U;
    if(str){
        if(*str == '=') str = omit_spaces(str + 1);
        const char *nxt = getnum(str, &U);
        if(nxt != str){ // setter
            CAN_sniffer((uint8_t)U);
        }
    }
    USB_sendstr("cansnif="); USB_putbyte('0' + cansniffer); newline();
    return ERR_OK;
}

static errcodes cansend(const char *txt){
    CAN_message canmsg;
    bzero(&canmsg, sizeof(canmsg));
    int ctr = -1;
    canmsg.ID = 0xffff;
    do{
        txt = omit_spaces(txt);
        uint32_t N;
        const char *n = getnum(txt, &N);
        if(txt == n) break;
        txt = n;
        if(ctr == -1){
            if(N > 0x7ff){
                return ERR_BADPAR;
            }
            canmsg.ID = (uint16_t)(N&0x7ff);
            ctr = 0;
            continue;
        }
        if(ctr > 7){
            return ERR_WRONGLEN;
        }
        if(N > 0xff){
            return ERR_BADVAL;
        }
        canmsg.data[ctr++] = (uint8_t) N;
    }while(1);
    if(canmsg.ID == 0xffff){
        return ERR_BADPAR;
    }
    canmsg.length = (uint8_t) ctr;
    uint32_t Tstart = Tms;
    while(Tms - Tstart < SEND_TIMEOUT_MS){
        if(CAN_OK == CAN_send(&canmsg)){
            //USB_sendstr("OK\n"); - don't send OK, as after sending might be an error
            return ERR_OK;
        }
    }
    return ERR_CANTRUN;
}

/************ END of all common functions list (for `textfunctions`) ************/

// in `textfn` arg `str` is rest of input string (spaces-omitted) after command
typedef errcodes (*textfn)(const char *str);
// array of text-only functions
static textfn textfunctions[TCMD_AMOUNT] = {
    [TCMD_PROHIBITED] = NULL,
    [TCMD_WDTEST] = wdtest,
    [TCMD_DUMPCONF] = dumpconf,
    [TCMD_CANSNIF] = cansnif,
    [TCMD_CANSEND] = cansend,
};

static char stbuf[256], *bptr = stbuf;
static int blen = 255;
static void initbuf(){bptr = stbuf; blen = 255; *bptr = 0;}
static void bufputchar(char c){
    if(blen == 0) return;
    *bptr++ = c; --blen;
    *bptr = 0;
}
static void add2buf(const char *s){
    if(!s) return;
    while(blen && *s){
        *bptr++ = *s++;
        --blen;
    }
    *bptr = 0;
}

static void printhelp(){
    const funcdescr *c = funclist;
    USB_sendstr("https://github.com/eddyem/stm32samples/tree/master/F3:F303/CANbus4BTA build#" BUILD_NUMBER " @ " BUILD_DATE "\n");
    USB_sendstr("commands format: parameter[number][=setter]\n");
    USB_sendstr("parameter [CAN idx] - help\n");
    USB_sendstr("--------------------------\n");
    while(c->help){
        if(!c->cmd){ // header
            USB_sendstr("\n    ");
            USB_sendstr(c->help);
            USB_putbyte(':');
        }else{
            USB_sendstr(c->cmd);
            if(c->idx > -1){
                USB_sendstr(" [");
                USB_sendstr(u2str(c->idx));
                USB_putbyte(']');
            }
            USB_sendstr(" - ");
            USB_sendstr(c->help);
        }
        newline();
        ++c;
    }
}

extern uint8_t usbON;

static const char* const errors_txt[ERR_AMOUNT] = {
     [ERR_OK] = "OK"
    ,[ERR_BADPAR] = "badpar"
    ,[ERR_BADVAL] = "badval"
    ,[ERR_WRONGLEN] = "wronglen"
    ,[ERR_BADCMD] = "badcmd"
    ,[ERR_CANTRUN] = "cantrun"
};

static void errtext(errcodes e){
    add2buf("error=");
    add2buf(errors_txt[e]);
    bufputchar('\n');
}

/**
 * @brief run_text_cmd - run text-only command
 * @param str - command string
 * @return NULL or string to send
 * WARNING! Sending help works only for USB!
 */
char *run_text_cmd(const char *str){
    if(!str || !*str) goto ret;
    char cmd[MAXCMDLEN + 1];
    errcodes ecode = ERR_BADCMD;
    initbuf();
    int idx = CMD_AMOUNT;
    const funcdescr *c = funclist;
    int l = 0;
    str = omit_spaces(str);
    const char *ptr = str;
    while(*ptr > '@' && l < MAXCMDLEN){ cmd[l++] = *ptr++;}
    if(l == 0) goto ret;
    cmd[l] = 0;
    while(c->help){
        if(!c->cmd) continue;
        if(0 == strcmp(c->cmd, cmd)){
            idx = c->idx;
            break;
        }
        ++c;
    }
    if(idx == CMD_AMOUNT){ // didn't found
        // send help over USB
        printhelp();
        goto ret;
    }
    str = omit_spaces(ptr);
    if(idx < 0){ // text-only function
        ecode = textfunctions[-idx](str);
        goto ret;
    }
    // common CAN/text function: we need to form 8-byte data buffer
    CAN_message msg;
    bzero(&msg, sizeof(msg));
    uint8_t *data = msg.data;
    uint8_t datalen = 2; // only command for start
    *((uint16_t*)data) = (uint16_t)idx;
    data[2] = NO_PARNO; // no parameter number by default
    if(*str >= '0' && *str <= '9'){ // have parameter with number
        uint32_t N;
        ptr = getnum(str, &N);
        if(ptr != str){
            str = ptr;
            if(N <= 0x7F) data[2] = (uint8_t)N;
            else{ ecode = ERR_BADPAR; goto ret; }
        }
        datalen = 3;
    }
    str = omit_spaces(str);
    if(*str == '='){ // setter
        ++str;
        ptr = getint(str, ((int32_t*)&data[4]));
        if(str == ptr){
            ecode = ERR_BADVAL;
            goto ret;
        }
        data[2] |= SETTER_FLAG;
        datalen = 8;
    }
    msg.length = datalen;
    run_can_cmd(&msg);
    // now check error code
    ecode = data[3];
ret:
    if(ecode != ERR_OK) errtext(ecode);
    else if(idx > -1){ // parce all back for common functions
        if(msg.length != 8){
            return "OK\n"; // non setters/getters will just print "OK" if all OK
        }else{
            add2buf(cmd);
            data[2] &= ~SETTER_FLAG;
            if(data[2] != NO_PARNO) add2buf(u2str(data[2]));
            bufputchar('=');
            add2buf(i2str(*(int32_t*)&data[4]));
            bufputchar('\n');
        }
    }
    return stbuf;
}
