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

#include <stm32f3.h>
#include <string.h>
#include "i2c.h"
#include "strfunc.h"
#include "usb.h"
#include "version.inc"

#define LOCBUFFSZ       (32)
// local buffer for I2C data to send
static uint8_t locBuffer[LOCBUFFSZ];
extern volatile uint32_t Tms;

const char *helpstring =
        "https://github.com/eddyem/stm32samples/tree/master/F3:F303/I2C_scan build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
        "i0..2 - setup I2C with lowest..highest speed (7.7, 10 and 100kHz)\n"
        "Ia addr - set I2C address\n"
        "Ig - dump content of I2Cbuf\n"
        "Iw bytes - send bytes (hex/dec/oct/bin) to I2C\n"
        "IW bytes - the same over DMA\n"
        "Ir reg n - read n bytes from I2C reg\n"
        "I2 reg16 n - read n bytes from 16-bit register\n"
        "In n - just read n bytes\n"
        "IN n - the same but with DMA\n"
        "Is - scan I2C bus\n"
        "T - print current Tms\n"
;

static uint8_t i2cinited = 0;
TRUE_INLINE const char *setupI2C(const char *buf){
    buf = omit_spaces(buf);
    if(*buf < '0' || *buf > '2') return "Wrong speed\n";
    i2c_setup(*buf - '0');
    i2cinited = 1;
    return "OK\n";
}

static uint8_t I2Caddress = 0;
TRUE_INLINE const char *saI2C(const char *buf){
    uint32_t addr;
    if(!getnum(buf, &addr) || addr > 0x7f) return "Wrong address\n";
    I2Caddress = (uint8_t) addr << 1;
    USND("I2Caddr="); USND(uhex2str(addr)); newline();
    return "OK\n";
}
static void rdI2C(const char *buf, int is16){
    uint32_t N = 0;
    int noreg = 0; // write register  (==1 - just read, ==2 - -//- using DMA)
    const char *nxt = NULL;
    if(*buf == 'n'){
        ++buf;
        noreg = 1;
    }else if(*buf == 'N'){
        ++buf;
        noreg = 2;
    }else{
        nxt = getnum(buf, &N);
        if(!nxt || buf == nxt || N > 0xffff || (!is16 &&  N > 0xff)){
            USND("Bad register number\n");
            return;
        }
        buf = nxt;
    }
    uint16_t reg = N;
    nxt = getnum(buf, &N);
    if(!nxt || buf == nxt || N > LOCBUFFSZ){
        USND("Bad length (<=32)\n");
        return;
    }
    const char *erd = "Error reading I2C\n";
    if(noreg){ // don't write register
        if(noreg == 1){
            USND("Simple read:\n");
            if(!read_i2c(I2Caddress, locBuffer, N)){
                USND(erd);
                return;
            }
        }else{
            USND("Try to read using DMA .. ");
            if(!read_i2c_dma(I2Caddress, N)) USND(erd);
            else USND("OK\n");
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
    if(N == 0){ USND("OK\n"); return; }
    if(!noreg){USND("Register "); USND(uhex2str(reg)); USND(":\n");}
    hexdump(USB_sendstr, locBuffer, N);
}
// read N numbers from buf, @return 0 if wrong or none
TRUE_INLINE uint16_t readNnumbers(const char *buf){
    uint32_t D;
    const char *nxt;
    uint16_t N = 0;
    while((nxt = getnum(buf, &D)) && nxt != buf && N < LOCBUFFSZ){
        buf = nxt;
        locBuffer[N++] = (uint8_t) D&0xff;
        USND("add byte: "); USND(uhex2str(D&0xff)); USND("\n");
    }
    USND("Send "); USND(u2str(N)); USND(" bytes\n");
    return N;
}
static const char *wrI2C(const char *buf, int isdma){
    uint16_t N = readNnumbers(buf);
    int result = isdma ? write_i2c_dma(I2Caddress, locBuffer, N) :
                         write_i2c(I2Caddress, locBuffer, N);
    if(!result) return "Error writing I2C\n";
    return "OK\n";
}

const char *parse_cmd(const char *buf){
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
            else if(*buf == '2'){ rdI2C(buf + 1, 1); return NULL; }
            else if(*buf == 'n'){ rdI2C(buf, 0); return NULL; }
            else if(*buf == 'N'){ rdI2C(buf, 0); return NULL; }
            else if(*buf == 'w') return wrI2C(buf + 1, 0);
            else if(*buf == 'W') return wrI2C(buf + 1, 1);
            else if(*buf == 's'){ i2c_init_scan_mode(); return "Start scan\n"; }
            else if(*buf == 'g'){ i2c_bufdudump(); return NULL; }
            else return "Command should be 'Ia', 'Iw', 'Ir' or 'Is'\n";
        break;
    }
    // "short" commands
    if(buf[1]) return buf; // echo wrong data
    switch(*buf){
        case 'T':
            USND("T=");
            USND(u2str(Tms));
            newline();
        break;
        default: // help
            USND(helpstring);
        break;
    }
    return NULL;
}
