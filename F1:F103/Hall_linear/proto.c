/*
 * This file is part of the canonmanage project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "adc.h"
#include "canproto.h" // errors
#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "strfunc.h"
#include "usb.h"
#include "version.inc"

const char* helpmsg =
    "https://github.com/eddyem/stm32samples/tree/master/F1-nolib/Hall_linear  build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
    "a - get ADC value (raw*Mul/Div)\n"
    "d - change Div\n"
    "m - change Mul\n"
    "r - get Raw ADC value\n"
    "u - get ADC voltage (*100)\n"
    "\t\tdebugging/conf commands:\n"
    "D - dump current config\n"
    "E - erase full flash storage\n"
    "F - reinit flashstorage\n"
    "I - set CAN ID (11 bit)\n"
    "M - show MCU temperature (*10)\n"
    "R - software reset\n"
    "S - set CAN speed (9600-3000000 baud)\n"
    "T - show Tms value\n"
    "V - get Vdd (*100)\n"
    "X - save current config to flash\n"
;

static const char* const errors_txt[ERR_AMOUNT] = {
     [ERR_OK] = "OK"
    ,[ERR_BADPAR] = "badpar"
    ,[ERR_BADVAL] = "badval"
    ,[ERR_WRONGLEN] = "wronglen"
    ,[ERR_BADCMD] = "badcmd"
    ,[ERR_CANTRUN] = "cantrun"
};

static void errtext(errcodes e){
    if(e != ERR_OK) USB_sendstr("error=");
    USB_sendstr(errors_txt[e]);
    newline();
}


void parse_cmd(const char *buf){
    int ret = ERR_CANTRUN;
    if(!buf || *buf == 0) return;
    uint32_t U, nan = 0;
    if(buf[1] == '\n' || !buf[1]){ // one symbol commands
        switch(*buf){
            case 'D': // dump current config
                dump_userconf();
                return;
            break;
            case 'E': // erase full flash storage
                if(0 == erase_storage(-1)) ret = ERR_OK;
            break;
            case 'F': // reinit flashstorage
                flashstorage_init();
                ret = ERR_OK;
            break;
            case 'M':
                USB_sendstr("MCUt=");
                printu(getMCUtemp());
                newline(); return;
            break;
            case 'R': // software reset
                USB_sendstr("Soft reset\n");
                NVIC_SystemReset();
            break;
            case 'T': // show Tms value
                USB_sendstr("Tms=");
                printu(Tms);
                newline(); return;
            break;
            case 'V':
                USB_sendstr("VDD=");
                printu(getVdd());
                newline(); return;
            break;
            case 'X': // save current config to flash
                if(0 == store_userconf()) ret = ERR_OK;
            break;
            case 'a': // get ADC value (raw*Mul/Div)
                U = getADCval(ADC_CH_0) * the_conf.Mul;
                USB_sendstr("ADC="); printu(U/the_conf.Div);
                newline(); return;
            break;
            case 'r': // get Raw ADC value
                USB_sendstr("ADCraw=");
                printu(getADCval(ADC_CH_0));
                newline(); return;
            break;
            case 'u': // get ADC voltage
                USB_sendstr("ADCv=");
                printu(getADCvoltage(ADC_CH_0));
                newline(); return;
            break;
            default:
                USB_sendstr(helpmsg);
                return;
        }
        goto ret;
    }
    char cmd = *buf++;
    const char *nxt = getnum(buf, &U);
    if(nxt == buf){
        nan = 1;
        ret = ERR_BADVAL;
    }
    switch(cmd){ // long messages
        case 'I': // can ID
            if(nan) break;
            else if(U >= 0x800) ret = ERR_WRONGLEN;
            else{
                the_conf.canID = U;
                ret = ERR_OK;
            }
        break;
        case 'S': // can speed
            if(nan) break;
            if(U < CAN_MIN_SPEED || U > CAN_MAX_SPEED) ret = ERR_WRONGLEN;
            else{
                the_conf.canspeed = U;
                ret = ERR_OK;
            }
        break;
        case 'd': // Divisor
            if(nan) break;
            if(U < 1 ) ret = ERR_WRONGLEN;
            else{
                the_conf.Div = U;
                ret = ERR_OK;
            }
        break;
        case 'm': // Multiplier
            if(nan) break;
            if(U < 1 || U > 1<<20) ret = ERR_WRONGLEN;
            else{
                the_conf.Mul = U;
                ret = ERR_OK;
            }
        break;
        default:
            USB_sendstr(helpmsg);
            return;
    }
ret:
    errtext(ret);
}


