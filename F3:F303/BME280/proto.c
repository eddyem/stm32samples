/*
 * This file is part of the bme280 project.
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

#include <stm32f3.h>
#include <string.h>

#include "BMP280.h"
#include "i2c.h"
#include "spi.h"
#include "strfunc.h"
#include "usb_dev.h"
#include "version.inc"

#define LOCBUFFSZ       (32)
// local buffer for SPI data to send
static uint8_t locBuffer[LOCBUFFSZ];
static const char *ERR = "ERR\n", *OK = "OK\n";
extern volatile uint32_t Tms;
int contmeas = 0; // continuous measurements each 1s

const char *helpstring =
        "https://github.com/eddyem/stm32samples/tree/master/F3:F303/xxx build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
        "c - set/reset continuous measurements\n"
        "s - send up to 32 bytes of data over SPI and read answer\n"
        "Axi - init BME280 with address x (0/1) and interface i (I/S - I2C/SPI)\n"
        "Fx- set filter to x (0..4)\n"
        "I - [re]init I2C\n"
        "Is - scan I2C bus\n"
        "M - start measurement\n"
        "Ovx - set oversampling of v(t, h or p) to x(0..5)\n"
        "S - [re]init SPI1\n"
        "T - print current Tms\n"
;

// read N numbers from buf, @return 0 if wrong or none
TRUE_INLINE uint16_t readNnumbers(const char *buf){
    uint32_t D;
    const char *nxt;
    uint16_t N = 0;
    while((nxt = getnum(buf, &D)) && nxt != buf && N < LOCBUFFSZ){
        if(D > 0xff){ USND("Each number should be uint8_t"); return 0; }
        buf = nxt;
        locBuffer[N++] = (uint8_t) D&0xff;
        //USND("add byte: "); USND(uhex2str(D&0xff)); USND("\n");
    }
    //USND("Send "); USND(u2str(N)); USND(" bytes\n");
    return N;
}

TRUE_INLINE const char* spirdwr(const char *buf){
    if(spi_status != SPI_READY){ USND("SPI not ready!"); return NULL; }
    uint16_t got = readNnumbers(buf);
    if(got < 1){ USND("Enter at least one uint8_t"); return NULL; }
    if(!spi_writeread(locBuffer, got)) return ERR;
    USND("Read data:");
    hexdump(USB_sendstr, locBuffer, got);
    return NULL;
}

TRUE_INLINE const char* bmeinint(const char *buf){
    buf = omit_spaces(buf);
    char c = *buf;
    if(c != '0' && c != '1') return "Wrong address\n";
    uint8_t addr = c - '0';
    buf = omit_spaces(buf + 1);
    c = *buf;
    uint8_t isI2C = 0;
    switch(c){
        case 'i':
        case 'I':
            isI2C = 1;
        case 's':
        case 'S':
        break;
        default:
            return "Wrong interface\n";
    }
    BMP280_setup(addr, isI2C);
    if(BMP280_init()){
        if(BMP280_read_ID(&addr)){
            if(addr == BMP280_CHIP_ID) return "found BMP280\n";
            else return "found BME280\n";
        }else return "Failed read ID\n";
    }
    return ERR;
}

TRUE_INLINE const char* setos(const char *buf){
    buf = omit_spaces(buf);
    uint32_t U32;
    void (*osfunc)(BMP280_Oversampling) = NULL;
    switch(*buf){
        case 'h': // h_os
        case 'H':
            osfunc = BMP280_setOSh;
        break;
        case 'p': // p_os
        case 'P':
            osfunc = BMP280_setOSp;
        break;
        case 't': // t_os
        case 'T':
            osfunc = BMP280_setOSt;
        break;
        default:
            return "Wrong OS function\n";
    }
    buf = omit_spaces(buf+1);
    if(!*buf) return "Need OS number\n";
    if(buf != getnum(buf, &U32) && U32 < BMP280_OVERSMAX){
        osfunc((BMP280_Oversampling)U32);
        return OK;
    }
    return "Wrong OS number\n";
}

const char *parse_cmd(const char *buf){
    uint32_t U32;
    // "long" commands
    if(buf[1]){
        char c = *buf++;
        switch(c){
            case 's':
                return spirdwr(buf);
            case 'A':
                return bmeinint(buf);
            case 'F':
                buf = omit_spaces(buf);
                if(buf != getnum(buf, &U32) && U32 < BMP280_FILTERMAX){
                    BMP280_setfilter((BMP280_Filter)U32);
                    return OK;
                }else return ERR;
            case 'I':
                if(*buf == 's'){
                    i2c_init_scan_mode();
                    return "Start scan\n";
                }else return ERR;
            case 'O':
                return setos(buf);
            default: return buf; // echo input
        }
    }
    // "short" commands
    switch(*buf){
        case 'c':
            contmeas = !contmeas;
            if(contmeas){
                if(BMP280_start()) return OK;
                return ERR;
            }
            return OK;
        case 'I':
            i2c_setup(I2C_SPEED_400K);
            return OK;
        case 'M':
            if(!BMP280_start()) return ERR;
            else return OK;
        break;
        case 'S':
            spi_setup();
            return OK;
        case 'T':
            USB_sendstr("T=");
            USB_sendstr(u2str(Tms));
            newline();
        break;
        default: // help
            USB_sendstr(helpstring);
        break;
    }
    return NULL;
}
