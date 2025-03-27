/*
 * This file is part of the encoders project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "flash.h"
#include "proto.h"
#include "spi.h"
#include "strfunc.h"
#include "usb_dev.h"
#include "version.inc"

// error codes
typedef enum {
    ERR_OK,         // no errors
    ERR_FAIL,       // failed to run command
    ERR_BADCMD,     // bad command
    ERR_BADPAR,     // bad parameter (absent or wrong value)
    ERR_SILENCE,    // getter or other - don't need to send "OK"
    ERR_AMOUNT
} errcode_e;

static const char* const errors[ERR_AMOUNT] = {
    [ERR_OK] = "OK",
    [ERR_FAIL] = "FAIL",
    [ERR_BADCMD] = "BADCMD",
    [ERR_BADPAR] = "BADPAR",
    [ERR_SILENCE] = "",
};

// commands indexes
typedef enum{
    C_dummy,
    C_help,
    C_sendX,
    C_sendY,
    C_setiface1,
    C_setiface2,
    C_setiface3,
    C_dumpconf,
    C_erasestorage,
    C_storeconf,
    C_reset,
    C_fin,
    C_encstart,
    C_encX,
    C_encY,
    C_spistat,
    C_spiinit,
    C_spideinit,
    C_AMOUNT
} cmd_e;

// command handler (idx - index of command in list, par - all after equal sign in "cmd = par")
typedef errcode_e (*handler_t)(cmd_e idx, char *par);

typedef struct{
    const char* cmd;    // command
    handler_t handler;  // handler function
} funcdescr_t;

static const funcdescr_t commands[C_AMOUNT];

// getter for integer parameter
#define IGETTER(idx, ival)  do{CMDWR(commands[idx].cmd); USB_putbyte(I_CMD, '='); \
                            CMDWR(i2str(ival)); CMDn(); return ERR_SILENCE; }while(0)

static errcode_e help(cmd_e idx, char* par);
static errcode_e dumpconf(cmd_e idx, char *par);

static errcode_e dummy(cmd_e idx, char *par){
    static int32_t val = -111;
    if(par && *par){
        int32_t i;
        char *nxt = getint(par, &i);
        DBGs(par);
        DBGs(nxt);
        if(par == nxt) return ERR_BADPAR;
        val = i;
    }
    IGETTER(idx, val);
}

static errcode_e sendenc(cmd_e idx, char *par){
    if(!par || !*par) return ERR_BADPAR;
    switch(idx){
        case C_sendX:
            if(!CDCready[I_X]) return ERR_FAIL;
            if(!USB_sendstr(I_X, par)) return ERR_FAIL;
            newline(I_X);
        break;
        case C_sendY:
            if(!CDCready[I_Y]) return ERR_FAIL;
            if(!USB_sendstr(I_Y, par)) return ERR_FAIL;
            newline(I_Y);
        break;
        default:
            return ERR_BADCMD;
    }
    return ERR_OK;
}

static errcode_e setiface(cmd_e idx, char *par){
    if(idx < C_setiface1 || idx >= C_setiface1 + bTotNumEndpoints) return ERR_BADCMD;
    idx -= C_setiface1; // now it is an index of iIlengths
    if(par && *par){
        int l = strlen(par);
        DBGs(i2str(l));
        if(l > MAX_IINTERFACE_SZ) return ERR_BADPAR; // too long
        the_conf.iIlengths[idx] = (uint8_t) l * 2;
        char *ptr = (char*)the_conf.iInterface[idx];
        for(int i = 0; i < l; ++i){ // make fucking hryunicode
            *ptr++ = *par++;
            *ptr++ = 0;
        }
    }
    // getter
    int l = the_conf.iIlengths[idx] / 2;
    char *ptr = (char*)the_conf.iInterface[idx];
    CMDWR(commands[idx + C_setiface1].cmd);
    CMDWR("=");
    for(int i = 0; i < l; ++i){
        USB_putbyte(I_CMD, *ptr);
        ptr += 2;
    }
    CMDn();
    return ERR_SILENCE;
}

static errcode_e erasestor(cmd_e _U_ idx, char *par){
    int32_t npage = -1;
    if(par){
        if(par == getint(par, &npage)) return ERR_BADPAR;
    }
    if(erase_storage(npage)) return ERR_FAIL;
    return ERR_OK;
}

static errcode_e storeconf(_U_ cmd_e idx, _U_ char *par){
    if(store_userconf()) return ERR_FAIL;
    return ERR_OK;
}

static errcode_e reset(_U_ cmd_e idx, _U_ char *par){
    NVIC_SystemReset();
    return ERR_OK; // never reached
}

static errcode_e fini(_U_ cmd_e idx, _U_ char *par){
    flashstorage_init();
    return ERR_OK; // never reached
}

static errcode_e encstart(cmd_e idx, _U_ char *par){
    if(idx == C_encX || idx == C_encstart)
        if(!spi_start_enc(0)) return ERR_FAIL;
    if(idx == C_encY || idx == C_encstart)
        if(!spi_start_enc(1)) return ERR_FAIL;
    return ERR_OK;
}

static errcode_e spistat(_U_ cmd_e idx, _U_ char *par){
    for(int i = 1; i < 3; ++i){
        USB_sendstr(I_CMD, "SPI");
        USB_putbyte(I_CMD, '0' + i);
        USB_sendstr(I_CMD, ": ");
        switch(spi_status[i]){
            case SPI_NOTREADY:
                USB_sendstr(I_CMD, "not ready");
            break;
            case SPI_BUSY:
                USB_sendstr(I_CMD, "busy");
            break;
            case SPI_READY:
                USB_sendstr(I_CMD, "ready");
            break;
        }
            newline(I_CMD);
    }
    return ERR_SILENCE;
}

static errcode_e spiinit(_U_ cmd_e idx, _U_ char *par){
    for(int i = 1; i < 3; ++i){
        USB_sendstr(I_CMD, "Init SPI");
        USB_putbyte(I_CMD, '0' + i);
        newline(I_CMD);
        spi_setup(i);
    }
    return ERR_SILENCE;
}

static errcode_e spideinit(_U_ cmd_e idx, _U_ char *par){
    for(int i = 1; i < 3; ++i){
        USB_sendstr(I_CMD, "DEinit SPI");
        USB_putbyte(I_CMD, '0' + i);
        newline(I_CMD);
        spi_deinit(i);
    }
    return ERR_SILENCE;
}

// text commands
static const funcdescr_t commands[C_AMOUNT] = {
    [C_dummy] = {"dummy", dummy},
    [C_help] = {"help", help},
    [C_sendX] = {"sendx", sendenc},
    [C_sendY] = {"sendy", sendenc},
    [C_setiface1] = {"setiface1", setiface},
    [C_setiface2] = {"setiface2", setiface},
    [C_setiface3] = {"setiface3", setiface},
    [C_dumpconf] = {"dumpconf", dumpconf},
    [C_erasestorage] = {"erasestorage", erasestor},
    [C_storeconf] = {"storeconf", storeconf},
    [C_reset] = {"reset", reset},
    [C_fin] = {"fin", fini},
    [C_encstart] = {"readenc", encstart},
    [C_encX] = {"readX", encstart},
    [C_encY] = {"readY", encstart},
    [C_spistat] = {"spistat", spistat},
    [C_spiinit] = {"spiinit", spiinit},
    [C_spideinit] = {"spideinit", spideinit},
};

static errcode_e dumpconf(cmd_e _U_ idx, char _U_ *par){
    CMDWR("userconf_sz="); CMDWR(u2str(the_conf.userconf_sz));
    CMDWR("\ncurrentconfidx="); CMDWR(i2str(currentconfidx));
    CMDn();
    for(int i = 0; i < bTotNumEndpoints; ++i)
        setiface(C_setiface1 + i, NULL);
    return ERR_SILENCE;
}

typedef struct{
    int idx;            // command index (if < 0 - just display `help` as grouping header)
    const char *help;   // help message
} help_t;

// SHOUL be sorted and grouped
static const help_t helpmessages[] = {
    {-1, "Configuration"},
    {C_dumpconf, "dump current configuration"},
    {C_erasestorage, "erase full storage or current page (=n)"},
    {C_setiface1, "set name of first (command) interface"},
    {C_setiface2, "set name of second (axis X) interface"},
    {C_setiface3, "set name of third (axis Y) interface"},
    {C_storeconf, "store configuration in flash memory"},
    {-1, "Different commands"},
    {C_dummy, "dummy integer setter/getter"},
    {C_encstart, "start reading encoders"},
    {C_encX, "read only X encoder"},
    {C_encY, "read only Y encoder"},
    {C_help, "show this help"},
    {C_reset, "reset MCU"},
    {C_spideinit, "deinit SPI"},
    {C_spiinit, "init SPI"},
    {C_spistat, "get status of both SPI interfaces"},
    {-1, "Debug"},
    {C_sendX, "send text string to X encoder's terminal"},
    {C_sendY, "send text string to Y encoder's terminal"},
    {C_fin, "reinit flash"},
    {-1, NULL},
};

static errcode_e help(_U_ cmd_e idx, _U_ char*  par){
    CMDWRn("https://github.com/eddyem/stm32samples/tree/master/F1:F103/BISS_C_encoders build #" BUILD_NUMBER " @ " BUILD_DATE);
    CMDWRn("\ncommands format: 'command[=setter]\\n'");
    const help_t *c = helpmessages;
    while(c->help){
        if(c->idx < 0){ // header
            CMDWR("\n    ");
            CMDWR(c->help);
            USB_putbyte(I_CMD, ':');
        }else{
            CMDWR(commands[c->idx].cmd);
            CMDWR(" - ");
            CMDWR(c->help);
        }
        CMDn();
        ++c;
    }
    return ERR_SILENCE;
}

// *cmd is "command" for commands/getters or "parameter=value" for setters
void parse_cmd(char *cmd){
    errcode_e ecode = ERR_BADCMD;
    // command and its parameter
    char *cmdstart = omit_spaces(cmd), *parstart = NULL;
    if(!cmdstart) goto retn;
    char *ptr = cmdstart;
    while(*ptr > ' ' && *ptr != '=') ++ptr;
    if(*ptr && *ptr <= ' '){ // there's spaces after command
        *ptr++ = 0;
        ptr = omit_spaces(ptr);
    }
    if(*ptr){ // check rest of string
        if(*ptr != '=') goto retn; // strange symbols after command name
        *ptr++ = 0;
        parstart = omit_spaces(ptr); // get parameter
        if(*parstart <= ' ') goto retn; // no parameter after equal sign
    }
    int idx = 0;
    for(; idx < C_AMOUNT; ++idx) // search in command lists
        if(0 == strcmp(commands[idx].cmd, cmdstart)) break;
    if(idx >= C_AMOUNT) goto retn; // not found
    ecode = commands[idx].handler(idx, parstart);
retn:
    if(ecode != ERR_SILENCE) CMDWRn(errors[ecode]);
}
