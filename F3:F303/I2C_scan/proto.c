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
#include "usb_dev.h"
#include "version.inc"

#define LOCBUFFSZ       (32)
// local buffer for I2C data to send
static uint8_t locBuffer[LOCBUFFSZ];
extern volatile uint32_t Tms;

static const char *OK = "OK\n";
static const char *helpstring =
        "https://github.com/eddyem/stm32samples/tree/master/F3:F303/I2C_scan build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
        "i0..3 - setup I2C with speed 10k, 100k, 400k, 1M or 2M (experimental!)\n"
        "B - switch to big-endian format for 16-bit registers\n"
        "Ia addr - set I2C address\n"
        "Ig - dump content of I2Cbuf\n"
        "Iw bytes - send bytes (hex/dec/oct/bin) to I2C\n"
        "Ir reg n - read n bytes from I2C reg\n"
        "I2 reg16 n - read n words from 16-bit register\n"
        "In n - just read n bytes\n"
        "Is - scan I2C bus\n"
        "-- note: all rw commands for 'I' could be started from 'D', meaning DMA operations --\n"
        "L - switch to little-endian (default) format for 16-bit registers\n"
        "T - print current Tms\n"
;

TRUE_INLINE const char *setupI2C(char *buf){
    if(!buf || !*buf){
        U("Current speed: "); USB_putbyte('0' + i2c_curspeed); newline();
        return NULL;
    }
    buf = omit_spaces(buf);
    int speed = *buf - '0';
    if(speed < 0 || speed >= I2C_SPEED_AMOUNT){
        return "Wrong speed!\n";
    }
    i2c_setup((i2c_speed_t)speed);
    return OK;
}

static uint8_t I2Caddress = 0;
TRUE_INLINE const char *saI2C(const char *buf){
    uint32_t addr;
    U("saI2C: '"); U(buf); U("'\n");
    const char *nxt = getnum(buf, &addr);
    if(nxt && nxt != buf){
        if(addr > 0x7f) return "Wrong address\n";
        I2Caddress = (uint8_t) addr << 1;
    }else addr = I2Caddress >> 1;
    U("I2Caddr="); USND(uhex2str(addr));
    return OK;
}
static void rdI2C(const char *buf, int is16, int dmaflag){
    uint32_t N = 0;
    int noreg = 0; // ==1 - just read (without sending regno)
    const char *nxt = NULL;
    if(*buf == 'n'){
        ++buf;
        noreg = 1;
    }else{
        nxt = getnum(buf, &N);
        if(!nxt || buf == nxt || N > 0xffff || (!is16 &&  N > 0xff)){
            USND("Bad register number");
            return;
        }
        buf = nxt;
    }
    uint16_t reg = N;
    nxt = getnum(buf, &N);
    uint32_t maxn = (is16) ? I2C_BUFSIZE / 2 : I2C_BUFSIZE;
    if(!nxt || buf == nxt || N > maxn){
        USND("Bad length");
        return;
    }
    const char *erd = "Error reading I2C\n";
    uint8_t *b8 = NULL; uint16_t *b16 = NULL;
    if(noreg){ // don't write register
        if(dmaflag){
            U("Try to read using DMA .. ");
            if(!read_i2c_dma(I2Caddress, N)) U(erd);
            else U(OK);
            return;
        }else{
            USND("Simple read:");
            if(!(b8 = read_i2c(I2Caddress, N))){
                U(erd);
                return;
            }
        }
    }else{
        if(is16){
            if(!(b16 = read_i2c_reg16(I2Caddress, reg, N, dmaflag))){
                U(erd);
                return;
            }
        }else{
            if(!(b8 = read_i2c_reg(I2Caddress, reg, N, dmaflag))){
                U(erd);
                return;
            }
        }
    }
    if(N == 0 || dmaflag){ U(OK); return; }
    if(!noreg){U("Register "); U(uhex2str(reg)); U(":\n");}
    if(is16) hexdump16(USB_sendstr, b16, N);
    else hexdump(USB_sendstr, b8, N);
}
// read N numbers from buf, @return 0 if wrong or none
TRUE_INLINE uint16_t readNnumbers(const char *buf){
    uint32_t D;
    const char *nxt;
    uint16_t N = 0;
    while((nxt = getnum(buf, &D)) && nxt != buf && N < LOCBUFFSZ){
        buf = nxt;
        locBuffer[N++] = (uint8_t) D&0xff;
        U("add byte: "); USND(uhex2str(D&0xff));
    }
    U("Send "); U(u2str(N)); USND(" bytes");
    return N;
}
static const char *wrI2C(const char *buf, int isdma){
    uint16_t N = readNnumbers(buf);
    if(N == 0) return "Enter at least one number\n";
    int result = isdma ? write_i2c_dma(I2Caddress, locBuffer, N) :
                         write_i2c(I2Caddress, locBuffer, N);
    if(!result) return "Error writing I2C\n";
    return OK;
}

const char *parse_cmd(char *buf){
    if(!buf || !*buf) return NULL;
    int dmaflag = 0;
    if(buf[1]){
        if(*buf == 'D'){
            dmaflag = 1; *buf = 'I'; // parse as for normal, but with DMA flag
        }
        switch(*buf){ // "long" commands
            case 'i':
                return setupI2C(buf + 1);
            break;
            case 'I':
                buf = omit_spaces(buf + 1);
                switch(*buf){
                    case 'a': return saI2C(buf + 1);
                    case 'r':
                        rdI2C(buf + 1, 0, dmaflag); return NULL;
                    case '2':
                        rdI2C(buf + 1, 1, dmaflag); return NULL;
                    case 'n':
                        rdI2C(buf, 0, dmaflag); return NULL;
                    case 'w': return wrI2C(buf + 1, dmaflag);
                    case 's':
                        i2c_init_scan_mode(); return "Start scan\n";
                    case 'g':
                        i2c_bufdudump(); return NULL;
                    default:
                        return "Wrong I2C command, read help!\n";
                }
            break;
            default:
                return("Wrong command, try '?' for help\n");
        }
    }
    switch(*buf){
        case 'i': return setupI2C(NULL); // current settings
        case 'B':
            endianness(1);
            return OK;
        break;
        case 'L':
            endianness(0);
            return OK;
        break;
        case 'T':
            U("T=");
            USND(u2str(Tms));
        break;
        default: // help
            U(helpstring);
        break;
    }
    return NULL;
}
