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
#include "modbusrtu.h"
#include "strfunc.h"
#include "usart.h"
#include "version.inc"

// constants with commands and flags
static const char* S_adc           = "adc";
static const char* S_bounce        = "bounce";
static const char* S_canbuserr     = "canbuserr";
static const char* S_canid         = "canid";
static const char* S_canidin       = "canidin";
static const char* S_canidout      = "canidout";
static const char* S_cansniff      = "cansniff";
static const char* S_canspeed      = "canspeed";
static const char* S_dumpconf      = "dumpconf";
static const char* S_eraseflash    = "eraseflash";
static const char* S_esw           = "esw";
static const char* S_eswnow        = "eswnow";
static const char* S_flags         = "flags";
static const char* S_inchannels    = "inchannels";
static const char* S_led           = "led";
static const char* S_mcutemp       = "mcutemp";
static const char* S_modbusid      = "modbusid";
static const char* S_modbusidout   = "modbusidout";
static const char* S_modbus        = "modbus";
static const char* S_modbusraw     = "modbusraw";
static const char* S_modbusspeed   = "modbusspeed";
static const char* S_outchannels   = "outchannels";
static const char* S_relay         = "relay";
static const char* S_reset         = "reset";
static const char* S_saveconf      = "saveconf";
static const char* S_s             = "s";
static const char* S_time          = "time";
static const char* S_usartspeed    = "usartspeed";
static const char* S_wdtest        = "wdtest";

// names of bit flags (ordered from LSE of[0])
static const char* S_f_relay_inverted      = "f_relay_inverted";
static const char* S_f_send_esw_can        = "f_send_esw_can";
static const char* S_f_send_relay_can      = "f_send_relay_can";
static const char* S_f_send_relay_modbus   = "f_send_relay_modbus";

// bitfield names should be in order of bit fields in confflags_t!
static const char ** const bitfields[] = {
    &S_f_send_esw_can,
    &S_f_send_relay_can,
    &S_f_relay_inverted,
    &S_f_send_relay_modbus,
    NULL
};


// runtime flags
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
    TCMD_MODBUS_SEND,   // send modbus request
    TCMD_MODBUS_SEND_RAW,// send raw modbus data (CRC added auto)
    // change bit flags
    TCMD_SW_SEND_CAN,   // change of IN will send its current state over CAN
    TCMD_SW_SEND_RCAN,  // change of IN will send command to change OUT by CAN
    TCMD_RELAY_INV,  // inverted state between relay and inputs when send over CAN/MODBUS
    TCMD_SW_SEND_RMODBUS,// modbus analog (if master)
    TCMD_AMOUNT
} text_cmd;

typedef struct{
    const char** const cmd; // command, if NULL - only display help message
    int idx;                // index in CAN cmd or text cmd list (if negative)
    const char* const help; // help message
} funcdescr;

// list of all text functions; should be sorted and can be grouped (`help` is header when cmd == NULL)
static const funcdescr funclist[] = {
//    {"adcmul", CMD_ADCMUL, "get/set ADC multipliers 0..3 (*1000)"},
//    {"adcraw", CMD_ADCRAW, "get raw ADC values of channel 0..4"},
//    {"adcv", CMD_ADCV,  "get ADC voltage of channel 0..3 (*100V)"},
//    {"bounce", CMD_BOUNCE, "get/set bounce constant (ms)"},
    {NULL, 0, "CAN bus commands"},
    {&S_canbuserr, -TCMD_CANBUSERRPRNT, "print all CAN bus errors (a lot of if not connected)"},
    {&S_cansniff, -TCMD_CANSNIFFER, "switch CAN sniffer mode"},
    {&S_s, -TCMD_CANSEND, "send CAN message: ID 0..8 data bytes"},
    {NULL, 0, "Configuration"},
    {&S_bounce, CMD_BOUNCE, "set/get anti-bounce timeout (ms, max: 1000)"},
    {&S_canid, CMD_CANID, "set both (in/out) CAN ID / get in CAN ID"},
    {&S_canidin, CMD_CANIDin, "get/set input CAN ID"},
    {&S_canidout, CMD_CANIDout, "get/set output CAN ID"},
    {&S_canspeed, CMD_CANSPEED, "get/set CAN speed (bps)"},
    {&S_dumpconf, -TCMD_DUMPCONF, "dump current configuration"},
    {&S_eraseflash, CMD_ERASESTOR, "erase all flash storage"},
    {&S_f_relay_inverted, -TCMD_RELAY_INV, "inverted state between relay and inputs"},
    {&S_f_send_esw_can, -TCMD_SW_SEND_CAN, "change of IN will send status over CAN with `canidin`"},
    {&S_f_send_relay_can, -TCMD_SW_SEND_RCAN, "change of IN will send also CAN command to change OUT with `canidout`"},
    {&S_f_send_relay_modbus, -TCMD_SW_SEND_RMODBUS, "change of IN will send also MODBUS command to change OUT with `modbusidout` (only for master!)"},
    {&S_flags, CMD_FLAGS, "set/get configuration flags (as one U32 without parameter or Nth bit with)"},
    {&S_modbusid, CMD_MODBUSID, "set/get modbus slave ID (1..247) or set it master (0)"},
    {&S_modbusidout, CMD_MODBUSIDOUT, "set/get modbus slave ID (0..247) to send relay commands"},
    {&S_modbusspeed, CMD_MODBUSSPEED, "set/get modbus speed (1200..115200)"},
    {&S_saveconf, CMD_SAVECONF, "save configuration"},
    {&S_usartspeed, CMD_USARTSPEED, "get/set USART1 speed"},
    {NULL, 0, "IN/OUT"},
    {&S_adc, CMD_ADCRAW, "get raw ADC values for given channel"},
    {&S_esw, CMD_GETESW, "anti-bounce read inputs"},
    {&S_eswnow, CMD_GETESWNOW, "read current inputs' state"},
    {&S_led, CMD_LED, "work with onboard LED"},
    {&S_relay, CMD_RELAY, "get/set relay state (0 - off, 1 - on)"},
    {NULL, 0, "Other commands"},
    {&S_inchannels, CMD_INCHNLS, "get u32 with bits set on supported IN channels"},
    {&S_mcutemp, CMD_MCUTEMP, "get MCU temperature (*10degrC)"},
    {&S_modbus, -TCMD_MODBUS_SEND, "send modbus request with format \"slaveID fcode regaddr nregs [N data]\", to send zeros you can omit rest of 'data'"},
    {&S_modbusraw, -TCMD_MODBUS_SEND_RAW, "send RAW modbus request (will send up to 62 bytes + calculated CRC)"},
    {&S_outchannels, CMD_OUTCHNLS, "get u32 with bits set on supported OUT channels"},
    {&S_reset, CMD_RESET, "reset MCU"},
    {&S_time, CMD_TIME, "get/set time (1ms, 32bit)"},
    {&S_wdtest, -TCMD_WDTEST, "test watchdog"},
    {NULL, 0, NULL} // last record
};

static void printhelp(){
    const funcdescr *c = funclist;
    usart_send("https://github.com/eddyem/stm32samples/tree/master/F1:F103/FX3U build #" BUILD_NUMBER " @ " BUILD_DATE "\n");
    usart_send("commands format: parameter[number][=setter]\n");
    usart_send("parameter [CAN idx] - help\n");
    usart_send("--------------------------\n");
    while(c->help){
        IWDG->KR = IWDG_REFRESH;
        if(!c->cmd){ // header
            usart_send("\n    ");
            usart_send(c->help);
            usart_putchar(':');
        }else{
            usart_send(*c->cmd);
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

/*********** START of all text functions list  ***********/

static errcodes cansnif(const char *str, text_cmd _U_ cmd){
    uint32_t U;
    if(str){
        if(*str == '=') str = omit_spaces(str + 1);
        const char *nxt = getnum(str, &U);
        if(nxt != str){ // setter
            CAN_sniffer((uint8_t)U);
        }
    }
    usart_send(S_cansniff); EQ(); usart_putchar('0' + flags.can_monitor); newline();
    return ERR_OK;
}

static errcodes canbuserr(const char *str, text_cmd _U_ cmd){
    uint32_t U;
    if(str){
        if(*str == '=') str = omit_spaces(str + 1);
        const char *nxt = getnum(str, &U);
        if(nxt != str){ // setter
            flags.can_printoff = U;
        }
    }
    usart_send(S_canbuserr); EQ(); usart_putchar('0' + flags.can_printoff); newline();
    return ERR_OK;
}

static errcodes wdtest(const char _U_ *str, text_cmd _U_ cmd){
    usart_send("Wait for reboot\n");
    usart_transmit();
    while(1){nop();}
    return ERR_OK;
}



static errcodes dumpconf(const char _U_ *str, text_cmd _U_ cmd){
#ifdef EBUG
    uint32_t sz = FLASH_SIZE*1024;
    usart_send("flashsize="); printu(sz); usart_putchar('/');
    printu(FLASH_blocksize); usart_putchar('='); printu(sz/FLASH_blocksize);
    usart_send(" blocks\n");
#endif
    usart_send("userconf_addr="); printuhex((uint32_t)Flash_Data);
    usart_send("\nuserconf_idx="); printi(currentconfidx);
    usart_send("\nuserconf_sz="); printu(the_conf.userconf_sz);
    newline(); usart_send(S_canspeed); EQ(); printu(the_conf.CANspeed);
    newline(); usart_send(S_canidin); EQ(); printu(the_conf.CANIDin);
    newline(); usart_send(S_canidout); EQ(); printu(the_conf.CANIDout);
    newline(); usart_send(S_usartspeed); EQ(); printu(the_conf.usartspeed);
    newline(); usart_send(S_modbusid); EQ(); printu(the_conf.modbusID);
    newline(); usart_send(S_modbusidout); EQ(); printu(the_conf.modbusIDout);
    newline(); usart_send(S_modbusspeed); EQ(); printu(the_conf.modbusspeed);
    newline(); usart_send(S_bounce); EQ(); printu(the_conf.bouncetime);
    const char ** const *p = bitfields;
    int bit = 0;
    newline(); usart_send(S_flags); printuhex(the_conf.flags.u32);
    while(*p){
        IWDG->KR = IWDG_REFRESH;
        newline(); usart_putchar(' ');
        usart_send(**p); EQ(); usart_putchar((the_conf.flags.u32 & (1<<bit)) ? '1' : '0');
        if(++bit > MAX_FLAG_BITNO) break;
        ++p;
    }
    newline();
    return ERR_OK;
}

static errcodes cansend(const char *txt, text_cmd _U_ cmd){
    CAN_message canmsg;
    bzero(&canmsg, sizeof(canmsg));
    int ctr = -1;
    canmsg.ID = 0xffff;
    do{
        IWDG->KR = IWDG_REFRESH;
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
        IWDG->KR = IWDG_REFRESH;
        if(CAN_OK == CAN_send(&canmsg)){
            return ERR_OK;
        }
    }
    return ERR_CANTRUN;
}

// change configuration flags by one
static errcodes confflags(const char _U_ *str, text_cmd cmd){
    if(str && *str){
        if(*str == '=') str = omit_spaces(str + 1);
        if(*str != '0' && *str != '1') return ERR_BADVAL;
        uint8_t val = *str - '0';
        switch(cmd){
            case TCMD_SW_SEND_CAN:
                the_conf.flags.sw_send_esw_can = val;
            break;
            case TCMD_SW_SEND_RCAN:
                the_conf.flags.sw_send_relay_can = val;
            break;
            case TCMD_RELAY_INV:
                the_conf.flags.sw_send_relay_inv = val;
            break;
            case TCMD_SW_SEND_RMODBUS:
                the_conf.flags.sw_send_relay_modbus = val;
            break;
            default:
                return ERR_BADCMD;
        }
    }
    uint8_t val = 0, idx = 0;
    switch(cmd){
        case TCMD_SW_SEND_CAN:
            val = the_conf.flags.sw_send_esw_can;
            idx = 0;
        break;
        case TCMD_SW_SEND_RCAN:
            val = the_conf.flags.sw_send_relay_can;
            idx = 1;
        break;
        case TCMD_RELAY_INV:
            val = the_conf.flags.sw_send_relay_inv;
            idx = 2;
        break;
        case TCMD_SW_SEND_RMODBUS:
            val = the_conf.flags.sw_send_relay_modbus;
            idx = 3;
        break;
        default:
            return ERR_BADCMD;
    }
    usart_send(*bitfields[idx]); EQ(); usart_putchar('0' + val);
    newline();
    return ERR_OK;
}

// format: slaveID Fcode startReg numRegs [N data];
// to send zeros you can omit rest of 'data'
static errcodes modbussend(const char *txt, text_cmd _U_ cmd){
    modbus_request req = {0};
    uint8_t reqdata[MODBUSBUFSZO - 7] = {0};
    uint32_t N = 0;
    const char *n = getnum(txt, &N);
    if(n == txt || N > MODBUS_MAX_ID){
        usart_send("Need slave ID (0..247)\n");
        return ERR_WRONGLEN;
    }
    req.ID = N;
    txt = n; n = getnum(txt, &N);
    if(n == txt || N > 127 || N == 0){
        usart_send("Need function code (1..127)\n");
        return ERR_WRONGLEN;
    }
    req.Fcode = N;
    txt = n; n = getnum(txt, &N);
    if(n == txt){
        usart_send("Need starting register address\n");
        return ERR_WRONGLEN;
    }
    req.startreg = N;
    txt = n; n = getnum(txt, &N);
    if(n == txt){
        usart_send("Need registers amount or data bytes\n");
        return ERR_WRONGLEN;
    }
    req.regno = N;
    txt = n; n = getnum(txt, &N);
    if((req.Fcode == MC_WRITE_MUL_COILS || req.Fcode == MC_WRITE_MUL_REGS)){ // request with data
        if(txt == n){
            usart_send("Need amount of data for given fcode\n");
            return ERR_WRONGLEN;
        }
        if(N == 0 || N > MODBUSBUFSZO - 7){
            usart_send("Data length too big\n");
            return ERR_BADVAL;
        }
        req.datalen = N;
        for(int i = 0; i < req.datalen; ++i){
            txt = n; n = getnum(txt, &N);
            if(txt == n) break;
            reqdata[i] = N;
        }
        req.data = reqdata;
    }else if(n != txt){
        usart_send("multiple data allows only for fcode 0x0f and 0x10; ignore other data\n");
    }
    if(modbus_send_request(&req) < 1) return ERR_CANTRUN;
    return ERR_OK;
}

// raw data format
static errcodes modbussendraw(const char *txt, text_cmd _U_ cmd){
    uint32_t N = 0;
    uint8_t reqdata[MODBUSBUFSZO], datalen = 0;
    for(; datalen < MODBUSBUFSZO; ++datalen){
        const char *n = getnum(txt, &N);
        if(n == txt) break;
        reqdata[datalen] = N;
        txt = n;
    }
    if(datalen == 0){
        usart_send("Need data bytes\n");
        return ERR_WRONGLEN;
    }
    if(modbus_send(reqdata, datalen) != datalen) return ERR_CANTRUN;
    return ERR_OK;
}

/************ END of all text functions list ************/

// in `textfn` arg `str` is the rest of input string (spaces-omitted) after command
typedef errcodes (*textfn)(const char *str, text_cmd cmd);
static textfn textfunctions[TCMD_AMOUNT] = {
    [TCMD_PROHIBITED] = NULL,
    [TCMD_WDTEST] = wdtest,
    [TCMD_DUMPCONF] = dumpconf,
    [TCMD_CANSEND] = cansend,
    [TCMD_CANSNIFFER] = cansnif,
    [TCMD_CANBUSERRPRNT] = canbuserr,
    [TCMD_MODBUS_SEND] = modbussend,
    [TCMD_MODBUS_SEND_RAW] = modbussendraw,
    [TCMD_SW_SEND_CAN] = confflags,
    [TCMD_SW_SEND_RCAN] = confflags,
    [TCMD_RELAY_INV] = confflags,
    [TCMD_SW_SEND_RMODBUS] = confflags,
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
    errcodes ecode = ERR_BADCMD;
    if(!str || !*str) goto ret;
    char cmd[MAXCMDLEN + 1];
    int idx = CMD_AMOUNT;
    const funcdescr *c = funclist;
    int l = 0;
    str = omit_spaces(str);
    const char *ptr = str;
    while(*ptr > '@' && l < MAXCMDLEN){ cmd[l++] = *ptr++;}
    if(l == 0) goto ret;
    cmd[l] = 0;
    while(c->help){
        IWDG->KR = IWDG_REFRESH;
        if(c->cmd && 0 == strcmp(*c->cmd, cmd)){
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
        ecode = textfunctions[-idx](str, -idx);
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
