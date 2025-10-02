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

#include "spi.h"
#include "strfunc.h"
#include "usb_dev.h"
#include "version.inc"

#define LOCBUFFSZ       (32)
// local buffer for SPI data to send
static uint8_t locBuffer[LOCBUFFSZ];
static const char *ERR = "ERR\n", *OK = "OK\n";
extern volatile uint32_t Tms;

const char *helpstring =
        "https://github.com/eddyem/stm32samples/tree/master/F3:F303/xxx build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
        "i - [re]init SPI1\n"
        "s - send up to 32 bytes of data and read answer\n"
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
        USND("add byte: "); USND(uhex2str(D&0xff)); USND("\n");
    }
    USND("Send "); USND(u2str(N)); USND(" bytes\n");
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

const char *parse_cmd(const char *buf){
    // "long" commands
    if(buf[1]){
        char c = *buf++;
        switch(c){
            case 's':
                return spirdwr(buf);
            default: return buf; // echo input
        }
    }
  /*  switch(*buf){
        case '':
        break;
    }*/
    // "short" commands
    switch(*buf){
        case 'i':
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
