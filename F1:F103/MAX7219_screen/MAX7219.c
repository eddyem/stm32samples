/*
 * This file is part of the MAX7219 project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "spi.h"
#include "MAX7219.h"
#ifdef EBUG
#include "usb.h"
#include "proto.h"
#endif

// all registers:
typedef enum {
    REG_NO_OP         = 0x00 << 8,
    REG_DIGIT_0       = 0x01 << 8,
    REG_DIGIT_1       = 0x02 << 8,
    REG_DIGIT_2       = 0x03 << 8,
    REG_DIGIT_3       = 0x04 << 8,
    REG_DIGIT_4       = 0x05 << 8,
    REG_DIGIT_5       = 0x06 << 8,
    REG_DIGIT_6       = 0x07 << 8,
    REG_DIGIT_7       = 0x08 << 8,
    REG_DECODE_MODE   = 0x09 << 8,
    REG_INTENSITY     = 0x0A << 8,
    REG_SCAN_LIMIT    = 0x0B << 8,
    REG_SHUTDOWN      = 0x0C << 8,
    REG_DISPLAY_TEST  = 0x0F << 8,
} MAX7219_REGISTERS;

typedef enum{
    S_RELAX,
    S_REFRESH,
} Status;

static uint8_t rdy2sendNXT = 1;

static Status s;

// screen buffer, sent line by line
static uint8_t screenbuf[8][NSCREENS];

// init controlling registers
static void inireg(uint16_t reg){
    CLEAR(CS);
    for(int i = 0; i < NSCREENS; ++i)
        SPI_blocktransmit(reg);
    SET(CS);
}
// setup display
void MAX7219_setup(){
    SET(CS);
    rdy2sendNXT = 1;
    SPI_setup();
#ifdef EBUG
    for(int i = 0; i < NSCREENS; ++i){
        MAX7219_point(0 + 8*i, 7, 1);
        MAX7219_point(1 + 8*i, 6, 1);
        MAX7219_point(2 + 8*i, 5, 1);
        MAX7219_point(3 + 8*i, 4, 1);
        MAX7219_point(4 + 8*i, 3, 1);
        MAX7219_point(5 + 8*i, 2, 1);
        MAX7219_point(6 + 8*i, 1, 1);
        MAX7219_point(7 + 8*i, 0, 1);
    }
#endif
    // send !shutdown
    inireg(REG_SHUTDOWN | 1);
    inireg(REG_DECODE_MODE);
    inireg(REG_INTENSITY | 3); // low intensity by default
    inireg(REG_SCAN_LIMIT | 7); // show all digits
    s = S_RELAX;
   // s = S_REFRESH;
}

// change intensity
void MAX7219_setintens(uint8_t intens){
    while(!rdy2sendNXT);
    uint16_t tr = REG_INTENSITY | intens;
    CLEAR(CS);
    for(int i = 0; i < NSCREENS; ++i)
        SPI_blocktransmit(tr);
    SET(CS);
}

// send command "display test"
void MAX7219_test(int t){
    while(!rdy2sendNXT);
    uint16_t tr = REG_DISPLAY_TEST;
    if(t) tr |= 1;
    CLEAR(CS);
    for(int i = 0; i < NSCREENS; ++i)
        SPI_blocktransmit(tr);
    SET(CS);
}

void MAX7219_refresh(){
    s = S_REFRESH;
}

/**
 * @brief MAX7219_point - set or clear point
 * @param x,y - coordinates (x=0..(8*NSCREENS-1), y=0..7
 * @param sc  - clear (0) or set (!0) pixel
 * @return 0 if failed
 */
int MAX7219_point(int x, int y, int sc){
    if(x < 0 || x >= 8*NSCREENS) return 0;
    if(y < 0 || y > 7) return 0;
    int N = x / 8, n = 7 - (x - N*8); // nbyte, nbit
#ifdef EBUG
    USB_send("pix N="); USB_send(u2str(N));
    USB_send(", n="); USB_send(u2str(n));
    USB_send(", x="); USB_send(u2str(x));
    USB_send(", y="); USB_send(u2str(y));
    USB_send("\n");
#endif
    if(sc){ // set pixel
        screenbuf[y][N] |= 1<<n;
    }else{
        screenbuf[y][N] &= ~(1<<n);
    }
    return 1;
}

// state machine
void MAX7219_process(){
    static int linenum = 0; // current processing line number
    static uint16_t sendreg[NSCREENS];
    if(s == S_RELAX) return;
    if(rdy2sendNXT){ // send next line
        if(linenum == 8){
            linenum = 0;
            s = S_RELAX;
            return;
        }
        rdy2sendNXT = 0;
        for(int i = 0; i < NSCREENS; ++i){
            sendreg[i] = ((linenum + 1) << 8) | screenbuf[linenum][i];
        }
        ++linenum;
        CLEAR(CS);
        if(SPI_transmit(sendreg, NSCREENS)){ // error, start from 0
            linenum = 0;
            MAX7219_setup();
            return;
        }
    }else{
        if(SPI1->SR & SPI_SR_TXE && !(SPI1->SR & SPI_SR_BSY)){ // last data transmitted
            SET(CS);
            rdy2sendNXT = 1;
#ifdef EBUG
            USB_send("Nxt\n");
#endif
        }
    }
}

// roll screen by 1 pixel to the left
void MAX7219_rollscreen(){
    for(int y = 0; y < 8; ++y){
        uint8_t _1st = (screenbuf[y][0] & (1<<7)) ? 1:0;
        for(int n = NSCREENS-1; n > -1; --n){
            uint8_t val = screenbuf[y][n];
            screenbuf[y][n] = (val<<1) | _1st;
            _1st = (val & (1<<7)) ? 1:0;
        }
    }
}
