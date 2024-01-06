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
    TCMD_PROHIBITED
    ,TCMD_WDTEST
    ,TCMD_DUMPCONF
    ,TCMD_AMOUNT
} text_cmd;


typedef struct{
    const char *cmd;    // command, if NULL - only display help message
    int idx;            // index in CAN cmd or text cmd list (if negative)
    const char *help;   // help message
} funcdescr;

// list of all text functions; should be sorted and can be grouped
static const funcdescr funclist[] = {
    {"adcmul", CMD_ADCMUL, "get/set ADC multipliers 0..4 (*1000)"},
    {"adcraw", CMD_ADCRAW, "get raw ADC values of channel 0..4"},
    {"adcv", CMD_ADCV,  "get ADC voltage of channel 0..3 (*100V)"},
    {"canid", CMD_CANID, "get/set CAN ID"},
    {"canspeed", CMD_CANSPEED, "get/set CAN speed (bps)"},
    {"dumpconf", -TCMD_DUMPCONF, "dump current configuration"},
    {"eraseflash", CMD_ERASESTOR, "erase all flash storage"},
    {"mcutemp", CMD_MCUTEMP, "get MCU temperature (*10degrC)"},
    {"reset", CMD_RESET, "reset MCU"},
    {"saveconf", CMD_SAVECONF, "save configuration"},
    {"time", CMD_TIME, "get/set time (ms)"},
    {"wdtest", -TCMD_WDTEST, "test watchdog"},
    {NULL, 0, NULL} // last record
};

/*********** START of all common functions list (for `textfunctions`) ***********/

static errcodes wdtest(const char _U_ *str){
    USB_sendstr("Wait for reboot\n");
    while(1){nop();}
    return ERR_OK;
}

static errcodes dumpconf(const char _U_ *str){
#ifdef EBUG
    USB_sendstr("flashsize="); printu(FLASH_SIZE); USB_putbyte('*');
    printu(FLASH_blocksize); USB_putbyte('='); printu(FLASH_SIZE*FLASH_blocksize);
    newline();
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
//#define PROPNAME(nm)    do{newline(); USB_sendstr(nm); USB_putbyte(cur); USB_putbyte('=');}while(0)
//#undef PROPNAME
    newline();
    return ERR_OK;
}

/************ END of all common functions list (for `textfunctions`) ************/

// in `textfn` arg `str` is rest of input string (spaces-omitted) after command
typedef errcodes (*textfn)(const char *str);
// array of text-only functions
static textfn textfunctions[TCMD_AMOUNT] = {
     [TCMD_PROHIBITED] = NULL
    ,[TCMD_WDTEST] = wdtest
    ,[TCMD_DUMPCONF] = dumpconf
};

static char stbuf[256], *bptr = NULL;
static int blen = 0;
static void initbuf(){bptr = stbuf; blen = 255; *bptr = 0;}
static void bufputchar(char c){
    if(blen == 0) return;
    *bptr++ = c; --blen;
    *bptr = 0;
}
static void add2buf(const char *s){
    while(blen && *s){
        *bptr++ = *s++;
        --blen;
    }
    *bptr = 0;
}

static void printhelp(){
    const funcdescr *c = funclist;
    USB_sendstr("https://github.com/eddyem/stm32samples/tree/master/F3:F303/CANbus4BTA build#" BUILD_NUMBER " @ " BUILD_DATE "\n");
    USB_sendstr("commands format: parameter[number][=setter]");
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
    char cmd[MAXCMDLEN + 1];
    errcodes ecode = ERR_BADCMD;
    if(!str || !*str) goto ret;
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
    }
    if(idx == CMD_AMOUNT){ // didn't found
        // send help over USB
        printhelp();
        goto ret;
    }
    initbuf();
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
            add2buf(funclist[idx].cmd);
            data[2] &= ~SETTER_FLAG;
            if(data[2] != NO_PARNO) add2buf(u2str(data[2]));
            bufputchar('=');
            add2buf(i2str(*(int32_t*)&data[4]));
            bufputchar('\n');
        }
    }
    return stbuf;
}
