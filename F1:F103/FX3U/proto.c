/*
 * This file is part of the fx3u project.
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

#include "adc.h"
#include "can.h"
#include "canproto.h"
#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "strfunc.h"
#include "usart.h"
#include "version.inc"

flags_t flags = {
    0
};

// text-only commans indexes (0 is prohibited)
typedef enum{
    TCMD_PROHIBITED,    // prohibited
    TCMD_WDTEST,        // test watchdog
    TCMD_DUMPCONF,      // dump configuration
    TCMD_CANSEND,       // send CAN message
    TCMD_CANSNIFFER,    // sniff all CAN messages
    TCMD_CANBUSERRPRNT, // pring all errors of bus
    TCMD_AMOUNT
} text_cmd;

typedef struct{
    const char *cmd;    // command, if NULL - only display help message
    int idx;            // index in CAN cmd or text cmd list (if negative)
    const char *help;   // help message
} funcdescr;

// list of all text functions; should be sorted and can be grouped (`help` is header when cmd == NULL)
static const funcdescr funclist[] = {
//    {"adcmul", CMD_ADCMUL, "get/set ADC multipliers 0..3 (*1000)"},
//    {"adcraw", CMD_ADCRAW, "get raw ADC values of channel 0..4"},
//    {"adcv", CMD_ADCV,  "get ADC voltage of channel 0..3 (*100V)"},
//    {"bounce", CMD_BOUNCE, "get/set bounce constant (ms)"},
    {NULL, 0, "CAN bus commands"},
    {"canbuserr", -TCMD_CANBUSERRPRNT, "print all CAN bus errors (a lot of if not connected)"},
    {"cansniff", -TCMD_CANSNIFFER, "switch CAN sniffer mode"},
    {NULL, 0, "Configuration"},
    {"bounce", CMD_BOUNCE, "set/get anti-bounce timeout (ms, max: 1000)"},
    {"canid", CMD_CANID, "set both (in/out) CAN ID / get in CAN ID"},
    {"canidin", CMD_CANIDin, "get/set input CAN ID"},
    {"canidout", CMD_CANIDout, "get/set output CAN ID"},
    {"canspeed", CMD_CANSPEED, "get/set CAN speed (bps)"},
    {"dumpconf", -TCMD_DUMPCONF, "dump current configuration"},
    {"eraseflash", CMD_ERASESTOR, "erase all flash storage"},
    {"saveconf", CMD_SAVECONF, "save configuration"},
    {"usartspeed", CMD_USARTSPEED, "get/set USART1 speed"},
    {NULL, 0, "IN/OUT"},
    {"adc", CMD_ADCRAW, "get raw ADC values for given channel"},
    {"esw", CMD_GETESW, "anti-bounce read inputs"},
    {"eswnow", CMD_GETESWNOW, "read current inputs' state"},
    {"led", CMD_LED, "work with onboard LED"},
    {"relay", CMD_RELAY, "get/set relay state (0 - off, 1 - on)"},
    {NULL, 0, "Other commands"},
    {"mcutemp", CMD_MCUTEMP, "get MCU temperature (*10degrC)"},
    {"reset", CMD_RESET, "reset MCU"},
    {"s", -TCMD_CANSEND, "send CAN message: ID 0..8 data bytes"},
    {"time", CMD_TIME, "get/set time (1ms, 32bit)"},
    {"wdtest", -TCMD_WDTEST, "test watchdog"},
    {NULL, 0, NULL} // last record
};

static void printhelp(){
    const funcdescr *c = funclist;
    usart_send("https://github.com/eddyem/stm32samples/tree/master/F1:F103/FX3U#" BUILD_NUMBER " @ " BUILD_DATE "\n");
    usart_send("commands format: parameter[number][=setter]\n");
    usart_send("parameter [CAN idx] - help\n");
    usart_send("--------------------------\n");
    while(c->help){
        if(!c->cmd){ // header
            usart_send("\n    ");
            usart_send(c->help);
            usart_putchar(':');
        }else{
            usart_send(c->cmd);
            if(c->idx > -1){
                usart_send(" [");
                usart_send(u2str(c->idx));
                usart_putchar(']');
            }
            usart_send(" - ");
            usart_send(c->help);
        }
        newline();
        ++c;
    }
}

/*********** START of all common functions list (for `textfunctions`) ***********/

static errcodes cansnif(const char *str){
    uint32_t U;
    if(str){
        if(*str == '=') str = omit_spaces(str + 1);
        const char *nxt = getnum(str, &U);
        if(nxt != str){ // setter
            CAN_sniffer((uint8_t)U);
        }
    }
    usart_send("cansniff="); usart_putchar('0' + flags.can_monitor); newline();
    return ERR_OK;
}

static errcodes canbuserr(const char *str){
    uint32_t U;
    if(str){
        if(*str == '=') str = omit_spaces(str + 1);
        const char *nxt = getnum(str, &U);
        if(nxt != str){ // setter
            flags.can_printoff = U;
        }
    }
    usart_send("canbuserr="); usart_putchar('0' + flags.can_printoff); newline();
    return ERR_OK;
}

static errcodes wdtest(const char _U_ *str){
    usart_send("Wait for reboot\n");
    usart_transmit();
    while(1){nop();}
    return ERR_OK;
}
/*
// names of bit flags (ordered from LSE of[0])
static const char * const bitfields[] = {
    "encisSSI",
    "emulatePEP",
    NULL
};*/

static errcodes dumpconf(const char _U_ *str){
#ifdef EBUG
    uint32_t sz = FLASH_SIZE*1024;
    usart_send("flashsize="); printu(sz); usart_putchar('/');
    printu(FLASH_blocksize); usart_putchar('='); printu(sz/FLASH_blocksize);
    usart_send(" blocks\n");
#endif
    usart_send("userconf_addr="); printuhex((uint32_t)Flash_Data);
    usart_send("\nuserconf_idx="); printi(currentconfidx);
    usart_send("\nuserconf_sz="); printu(the_conf.userconf_sz);
    usart_send("\ncanspeed="); printu(the_conf.CANspeed);
    usart_send("\ncanid_in="); printu(the_conf.CANIDin);
    usart_send("\ncanid_out="); printu(the_conf.CANIDout);
    /*for(int i = 0; i < ADC_TSENS; ++i){
        usart_send("\nadcmul"); usart_putchar('0'+i); usart_putchar('=');
        usart_send(float2str(the_conf.adcmul[i], 3));
    }*/
    usart_send("\nusartspeed="); printu(the_conf.usartspeed);
    usart_send("\nbouncetime="); printu(the_conf.bouncetime);
    /*
    const char * const *p = bitfields;
    int bit = 0;
    while(*p){
        newline();
        usart_send(*p); usart_putchar('='); usart_putchar((the_conf.flags & (1<<bit)) ? '1' : '0');
        if(++bit > 31) break;
        ++p;
    }*/
    newline();
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
            return ERR_OK;
        }
    }
    return ERR_CANTRUN;
}

/************ END of all common functions list (for `textfunctions`) ************/

// in `textfn` arg `str` is the rest of input string (spaces-omitted) after command
typedef errcodes (*textfn)(const char *str);
static textfn textfunctions[TCMD_AMOUNT] = {
    [TCMD_PROHIBITED] = NULL,
    [TCMD_WDTEST] = wdtest,
    [TCMD_DUMPCONF] = dumpconf,
    [TCMD_CANSEND] = cansend,
    [TCMD_CANSNIFFER] = cansnif,
    [TCMD_CANBUSERRPRNT] = canbuserr,
};

static const char* const errors_txt[ERR_AMOUNT] = {
    [ERR_OK] = "OK"
    ,[ERR_BADPAR] = "badpar"
    ,[ERR_BADVAL] = "badval"
    ,[ERR_WRONGLEN] = "wronglen"
    ,[ERR_BADCMD] = "badcmd"
    ,[ERR_CANTRUN] = "cantrun"
};

static void errtext(errcodes e){
    usart_send("error=");
    usart_send(errors_txt[e]);
    newline();
}

/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 */
void cmd_parser(const char *str){
    if(!str || !*str) goto ret;
    char cmd[MAXCMDLEN + 1];
    errcodes ecode = ERR_BADCMD;
    int idx = CMD_AMOUNT;
    const funcdescr *c = funclist;
    int l = 0;
    str = omit_spaces(str);
    const char *ptr = str;
    while(*ptr > '@' && l < MAXCMDLEN){ cmd[l++] = *ptr++;}
    if(l == 0) goto ret;
    cmd[l] = 0;
    while(c->help){
        if(c->cmd && 0 == strcmp(c->cmd, cmd)){
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
            usart_send("OK\n"); // non setters/getters will just print "OK" if all OK
        }else{
            usart_send(cmd);
            data[2] &= ~SETTER_FLAG;
            if(data[2] != NO_PARNO) usart_send(u2str(data[2]));
            usart_putchar('=');
            usart_send(i2str(*(int32_t*)&data[4]));
            newline();
        }
    }
}
