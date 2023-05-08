/*
 * This file is part of the nitrogen project.
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

#include "hardware.h"
#include "ili9341.h"
#include "spi.h"
#ifdef EBUG
#include "strfunc.h"
#endif
#include "usb.h"

static const uint8_t initcmd[] = {
    ILI9341_SWRESET,     0xff, // reset and wait a lot
    //0xEF, 3, 0x03, 0x80, 0x02, // WTF?
    ILI9341_POWCTLA,     5, 0x39, 0x2C, 0x00, 0x34, 0x02, // default
    ILI9341_POWCTLB,     3, 0x00, 0xC1, 0x30, // PC/EQ for power saving
    ILI9341_DRVTCTLA1,   3, 0x85, 0x00, 0x78, // EQ timimg
    ILI9341_DRVTCTLB,    2, 0x00, 0x00, // 0 units for gate drv. timing control
    ILI9341_POWONSEQCTL, 4, 0x64, 0x03, 0x12, 0x81, // - why not default?
    ILI9341_PUMPRATCTL,  1, 0x20, // DDVDH=2xVCI
    ILI9341_POWCTL(1),   1, 0x23, // Power control: 4.6V grayscale level
    ILI9341_POWCTL(2),   1, 0x10, // ?
    ILI9341_VCOMCTL1,    2, 0x3e, 0x28, // Vcomh=4.25V, Vcoml=-1.5V
    ILI9341_VCOMCTL2,    1, 0x86, // change VCOMH/L: (-58)
    ILI9341_MADCTL,      1, DEFMADCTL, // mem access
    ILI9341_VSCRSADD,    1, 0x00, // default
    ILI9341_COLMOD,      1, 0x55, // 16 bits/pix
    ILI9341_FRCTLN,      2, 0x00, 0x18, // 79Hz
    ILI9341_DFUNCTL,     3, 0x08, 0x82, 0x27, // ?
    ILI9341_3GEN,        1, 0x00, // 3Gamma Function Disable
    ILI9341_GAMMASET,    1, 0x01, // default
    ILI9341_POSGAMCOR,   15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, // Set Gamma
    0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
    ILI9341_NEGGAMCOR,   15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, // Set Gamma
    0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
    ILI9341_SLPOUT,      0x8a, // Exit Sleep
    ILI9341_NORON,       0x80, // Normal display mode ON
    ILI9341_DISPON,      0x80, // Display on
    0x00 // End of list
};

int ili9341_init(){
    uint8_t *ptr = (uint8_t*)initcmd;
    uint8_t reg;
    while((reg = *ptr++)){
        IWDG->KR = IWDG_REFRESH;
        uint8_t N = *ptr++;
        if(N & 0x80){
            if(!ili9341_writecmd(reg)) return 0;
            uint32_t T = Tms;
            uint32_t pause = (N & 0x7f) + 1;
            if(pause>1) while(Tms - T < pause);
            continue;
        }
        if(!ili9341_writereg(reg, ptr, N)) return 0;
        ptr += N;
    }
    return 1;
}

/**
 * @brief il9341_readreg - read data from register
 * @param reg - register
 * @param data (i) - data
 * @param N - length of data
 * @return 0 if failed
 */
int ili9341_readreg(uint8_t reg, uint8_t *data, uint32_t N){
    SCRN_Command();
    SCRN_RST_set(0);
    int r = 0;
    do{
        if(!spi_write(&reg, 1)) break;
        if(!spi_waitbsy()) break;
        SCRN_Data();
        if(!spi_read(data, N)) break;
        if(!spi_waitbsy()) break;
        r = 1;
    }while(0);
    SCRN_Command();
    SCRN_RST_set(1);
    return r;
}

/**
 * @brief il9341_writereg - write data to register
 * @param reg - register
 * @param data (o) - data
 * @param N - length of data
 * @return 0 if failed
 */
int ili9341_writereg(uint8_t reg, const uint8_t *data, uint32_t N){
    SCRN_Command();
    SCRN_RST_set(0);
    int r = 0;
    do{
        if(!spi_write(&reg, 1)) break;
        if(!spi_waitbsy()) break;
        SCRN_Data();
        if(!spi_write(data, N)) break;
        if(!spi_waitbsy()) break;
        r = 1;
    }while(0);
    SCRN_Command();
    SCRN_RST_set(1);
    return r;
}

// write register with uint16_t data (swap bytes)
int ili9341_writereg16(uint8_t reg, const uint16_t data){
    SCRN_Command();
    SCRN_RST_set(0);
    int r = 0;
    do{
        if(!spi_write(&reg, 1)) break;
        if(!spi_waitbsy()) break;
        SCRN_Data();
        uint16_t s = __builtin_bswap16(data);
        if(!spi_write((uint8_t*)&s, 2)) break;
        if(!spi_waitbsy()) break;
        r = 1;
    }while(0);
    SCRN_Command();
    SCRN_RST_set(1);
    return r;
}

int ili9341_writereg32(uint8_t reg, uint16_t data1, uint16_t data2){
    SCRN_Command();
    SCRN_RST_set(0);
    int r = 0;
    do{
        if(!spi_write(&reg, 1)) break;
        if(!spi_waitbsy()) break;
        SCRN_Data();
        uint16_t s = __builtin_bswap16(data1);
        if(!spi_write((uint8_t*)&s, 2)) break;
        s = __builtin_bswap16(data2);
        if(!spi_write((uint8_t*)&s, 2)) break;
        if(!spi_waitbsy()) break;
        r = 1;
    }while(0);
    SCRN_Command();
    SCRN_RST_set(1);
    return r;
}

// write simple command
int ili9341_writecmd(uint8_t cmd){
    SCRN_Command();
    SCRN_RST_set(0);
    int r = 0;
    do{
        if(!spi_write(&cmd, 1)) break;
        if(!spi_waitbsy()) break;
        r = 1;
    }while(0);
    SCRN_RST_set(1);
    return r;
}

// write data
int ili9341_writedata(const uint8_t *data, uint32_t N){
    SCRN_Data();
    SCRN_RST_set(0);
    int r = 0;
    do{
        if(!spi_write(data, N)) break;
        if(!spi_waitbsy()) break;
        r = 1;
    }while(0);
    SCRN_Command();
    SCRN_RST_set(1);
    return r;
}

static int fillcmd(uint16_t color, uint8_t cmd, int sz){
    uint16_t rc = __builtin_bswap16(color);
    //USB_sendstr("rc="); USB_sendstr(u2str(rc)); newline();
    SCRN_Command();
    SCRN_RST_set(0);
    int r = 0;
    if(!spi_write(&cmd, 1)) goto rtn;
    if(!spi_waitbsy()) goto rtn;
    r = 1;
    SCRN_Data();
    uint16_t black = 0;
    for(int i = 0; i < sz; ++i){
        IWDG->KR = IWDG_REFRESH;
        if(i%80 == 0){
            if(!spi_write((uint8_t*)&black, 2)){
                r = 0;
                break;
            }
        } else if(!spi_write((uint8_t*)&rc, 2)){
            r = 0;
            break;
        }
    }
    if(!spi_waitbsy()) r = 0;
rtn:
    SCRN_Command();
    SCRN_RST_set(1);
    return r;
}

// fill start
int ili9341_fill(uint16_t color){
    return fillcmd(color, ILI9341_RAMWR, SCRNSZ/4);
}
// fill next
int ili9341_filln(uint16_t color){
    return fillcmd(color, ILI9341_WRMEMCONT, SCRNSZ/4);
}
// fill part (sz < 0 - fill from beginning)
int ili9341_fillp(uint16_t color, int sz){
    if(sz < 0) return fillcmd(color, ILI9341_RAMWR, -sz);
    else return fillcmd(color, ILI9341_WRMEMCONT, sz);
}

// set column limits
int ili9341_setcol(uint16_t start, uint16_t stop){
    return ili9341_writereg32(ILI9341_CASET, start, stop);
}

// set row limits
int ili9341_setrow(uint16_t start, uint16_t stop){
    return ili9341_writereg32(ILI9341_PASET, start, stop);
}
