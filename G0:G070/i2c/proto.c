/*
 * This file is part of the i2cscan project.
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

#include <stm32g0.h>
#include "i2c.h"
#include "strfunc.h"
#include "usart.h"

#define LOCBUFFSZ       (32)
// local buffer for I2C data to send
static uint8_t locBuffer[LOCBUFFSZ];

const char *helpstring =
        "i0..2 - setup I2C with lowest..highest speed (7.7, 10 and 100kHz)\n"
        "Ia addr - set I2C address\n"
        "Iw bytes - send bytes (hex/dec/oct/bin) to I2C\n"
        "Ir reg n - read n bytes from I2C reg\n"
        "IR reg16 n - read n bytes from 16-bit register\n"
        "In n - just read n bytes\n"
        "Is - scan I2C bus\n"
        "U - send long buffer over USART\n"
;

const char *longbuff = "=======-10=======-20=======-30=======-40=======-50=======-60=======-70=======-80=======-90======-100\n";
static void U3sendlong(const char *str){
    int l = strlen(str);
    while(l){
        int sent = usart3_sendstr(str);
        str += sent;
        l -= sent;
    }
}

static uint8_t i2cinited = 0;
static inline char *setupI2C(char *buf){
    buf = omit_spaces(buf);
    if(*buf < '0' || *buf > '2') return "Wrong speed";
    i2c_setup(*buf - '0');
    i2cinited = 1;
    return "OK";
}

static uint8_t I2Caddress = 0;
static inline char *saI2C(char *buf){
    uint32_t addr;
    if(!getnum(buf, &addr) || addr > 0x7f) return "Wrong address";
    I2Caddress = (uint8_t) addr << 1;
    USND("I2Caddr="); USND(uhex2str(addr)); USND("\n");
    return "OK";
}
static inline void rdI2C(char *buf, int is16){
    uint32_t N;
    int noreg = 0;
    char *nxt = NULL;
    if(*buf != 'n'){
        nxt = getnum(buf, &N);
        if(!nxt || buf == nxt || N > 0xffff || (!is16 &&  N > 0xff)){
            USND("Bad register number\n");
            return;
        }
        buf = nxt;
    }else{
        ++buf;
        noreg = 1;
    }
    uint16_t reg = N;
    nxt = getnum(buf, &N);
    if(!nxt || buf == nxt || N > LOCBUFFSZ){
        USND("Bad length (<=32)\n");
        return;
    }
    const char *erd = "Error reading I2C\n";
    if(noreg){ // don't write register
        if(!read_i2c(I2Caddress, locBuffer, N)){
            USND(erd);
            return;
        }
    }else{
        if(is16){
            if(!read_i2c_reg16(I2Caddress, reg, locBuffer, N)){
                USND(erd);
                return;
            }
        }else{
            if(!read_i2c_reg(I2Caddress, reg, locBuffer, N)){
                USND(erd);
                return;
            }
        }
    }
    if(N == 0){ USND("OK"); return; }
    USND("Register "); USND(uhex2str(reg)); USND(":\n");
    hexdump(usart3_sendstr, locBuffer, N);
}
// read N numbers from buf, @return 0 if wrong or none
static uint16_t readNnumbers(char *buf){
    uint32_t D;
    char *nxt;
    uint16_t N = 0;
    while((nxt = getnum(buf, &D)) && nxt != buf && N < LOCBUFFSZ){
        buf = nxt;
        locBuffer[N++] = (uint8_t) D&0xff;
        USND("add byte: "); USND(uhex2str(D&0xff)); USND("\n");
    }
    USND("Send "); USND(u2str(N)); USND(" bytes\n");
    return N;
}
static inline char *wrI2C(char *buf){
    uint16_t N = readNnumbers(buf);
    if(!write_i2c(I2Caddress, locBuffer, N)) return "Error writing I2C";
    return "OK";
}


char *parse_cmd(char *buf){
    // "long" commands
    switch(*buf){
        case 'i':
            return setupI2C(buf + 1);
        break;
        case 'I':
            if(!i2cinited) return "Init I2C first";
            buf = omit_spaces(buf + 1);
            if(*buf == 'a') return saI2C(buf + 1);
            else if(*buf == 'r'){ rdI2C(buf + 1, 0); return NULL; }
            else if(*buf == 'R'){ rdI2C(buf + 1, 1); return NULL; }
            else if(*buf == 'n'){ rdI2C(buf, 0); return NULL; }
            else if(*buf == 'w') return wrI2C(buf + 1);
            else if(*buf == 's'){ i2c_init_scan_mode(); return "Start scan\n"; }
            else return "Command should be 'Ia', 'Iw', 'Ir' or 'Is'\n";
        break;
    }
    // "short" commands
    if(buf[1]) return buf; // echo wrong data
    switch(*buf){
        case 'U':
            U3sendlong(longbuff);
        break;
        default: // help
            U3sendlong(helpstring);
        break;
    }
    return NULL;
}
