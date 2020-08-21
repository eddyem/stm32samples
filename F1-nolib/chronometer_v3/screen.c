/*
 * This file is part of the LED_screen project.
 * Copyright 2019 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include <string.h> // memset, memcpy
#include <stdio.h>
#include "fonts.h"
#include "hardware.h"
#include "screen.h"
#include "spi.h"
#include "usart.h"

#undef DBG
#define DBG(x)

// !!!FOR LITTLE-ENDIAN!!!

// X coordinate - from left to right!
// Y coordinate - from top to bottom!
// (0,0) is top left corner

// all-screen buffer
static uint8_t screenbuf[SCREENBUF_SZ];
// buffers for DMA - for each of four parts
static uint8_t dmabuf[4][DMABUF_SZ];

/**
 * @brief FillScreen - fill screen buffer with 0 or 1
 * @param setclear   - !=1 to set & ==0 to reset
 */
void FillScreen(uint8_t setclear){
    uint8_t pattern = 0;
    if(setclear) pattern = 0xff;
    if(SCREEN_IS_NEGATIVE) pattern = ~pattern;
    for(int i = 0; i < SCREENBUF_SZ; ++i) screenbuf[i] = pattern;
        // memset -> halt
    //memset(screenbuf, pattern, SCREENBUF_SZ);
}

/**
 * @brief DrawPix - set or clear pixel
 * @param X, Y - pixel coordinates (could be outside of screen)
 * @param pix - == 1 to set and 0 to clear
 */
void DrawPix(int16_t X, int16_t Y, uint8_t pix){
    if(X < 0 || X > SCREEN_WIDTH-1 || Y < 0 || Y > SCREEN_HEIGHT-1) return; // outside of screen
    // now calculate coordinate of pixel
    uint8_t *ptr = &screenbuf[Y*SCREEN_WIDTH/8 + X/8];
    if(SCREEN_IS_NEGATIVE)pix = !pix;
    if(pix) *ptr |= 1 << (7 - (X%8));  // only for little-endian
    else *ptr &= ~(1 << (7 - (X%8))); // only for little-endian
}

/**
 * @brief DrawCharAt - draws character @ position X,Y (this point is left baseline corner of char!)
 * @param X, Y  - started point
  * @param Char - char to draw
 * @return char width
 */
uint8_t DrawCharAt(int16_t X, int16_t Y, uint8_t Char){
    const uint8_t *curchar = font_char(Char);
    if(!curchar) return 0;
    // now change Y coordinate to left upper corner of font
    Y += 1 - curfont->height + curfont->baseline;
    // height and width of letter in pixels
    uint8_t h = curfont->height, w = *curchar++; // now curchar is pointer to bits array
    uint8_t lw = curfont->bytes / h; // width of letter in bytes
    for(uint8_t row = 0; row < h; ++row){
        for(uint8_t col = 0; col < w; ++col){
            DrawPix(X + col, Y + row, curchar[row*lw + (col/8)] & (1 << (7 - (col%8)))); // only for little-endian
        }
    }
    return w;
}

/**
 * @brief GetStrWidth - get width of string (in pixels)
 * @param str - text string
 * @return amount of pixels
 */
uint16_t GetStrWidth(const char *str){
    uint16_t w = 0;
    while(*str){
        const uint8_t *curchar = font_char(*str);
        if(curchar){
            w += *curchar;
        }
        ++str;
    }
    return w;
}

/**
 * @brief ConvertScreenBuf - convert scscreenbuf into dmabuf
 *
 */
void ConvertScreenBuf(){
    for(uint8_t partNo = 0; partNo < 4; ++ partNo){ // cycle by strings
        uint8_t *dmaptr = dmabuf[partNo];
        for(int X = 0; X < SCREEN_WIDTH/8; ++X){
            for(int Y = SCREEN_HEIGHT-4+partNo; Y >= 0; Y -= 4){ // and cycle by Y
                *dmaptr++ = screenbuf[X + Y*(SCREEN_WIDTH/8)];
            }
        }
    }
}

/**
 * @brief PutStringAt - draw text string @ screen
 * @param X, Y - base coordinates
 * @param str  - string to draw
 * @return - text width in pixels
 */
uint8_t PutStringAt(int16_t X, int16_t Y, const char *str){
    if(!str) return 0;
    int16_t Xold = X;
    while(*str){
        X += DrawCharAt(X, Y, *str++);
    }
    return X - Xold;
}

uint8_t *getScreenBuf(){return screenbuf;}
uint8_t *getDmaBuf(uint8_t N){
    if(N > 3) return NULL;
    return dmabuf[N];
}

extern volatile uint32_t Tms;
typedef enum{ // screen states
    SCREEN_RELAX,       // nothing to do (screen is off)
    SCREEN_SPIACTIVE,   // SPI transmission active
    SCREEN_WAIT,        // pause - current quarter is ON
    SCREEN_UPDATENXT    // update next quarter
} screen_state;

static screen_state ScrnState = SCREEN_RELAX;

/**
 * @brief process_screen - screen state machine processing
 */
void process_screen(){
    static uint32_t Tscr_last = 0;
    static uint8_t currentQ = 0; // current quarter
    switch(ScrnState){
        case SCREEN_SPIACTIVE: // SPI transmission active
            if(SPI_status == SPI_READY){
                DBG("SPI ready\n");
                Tscr_last = Tms;
                ScrnState = SCREEN_WAIT;
                SET(SCLK); // lock data
            }
        break;
        case SCREEN_WAIT: // wait
            if(Tms - Tscr_last > SCREEN_PAUSE){
                DBG("Pause ends\n");
                ScrnState = SCREEN_UPDATENXT;
            }
        break;
        case SCREEN_UPDATENXT:
            if(SPI_status == SPI_NOTREADY){
                DBG("SPI not ready - setup");
                spi_setup();
                return;
            }
            if(SPI_status != SPI_READY){
                DBG("SPI busy");
                return; // SPI not ready - try next time
            }
            if(SPI_transmit(dmabuf[currentQ], DMABUF_SZ)){
                DBG("SPI error");
                return; // transmission error - try next time
            }
            DBG("Send next");
            ScrnState = SCREEN_SPIACTIVE;
            // now prepare selectors
            CLEAR(SCLK);
            switch(currentQ){ // set address bits
                case 0:
                    CLEAR(A);
                    CLEAR(B);
                break;
                case 1:
                    SET(A);
                    CLEAR(B);
                break;
                case 2:
                    CLEAR(A);
                    SET(B);
                break;
                case 3:
                    SET(A);
                    SET(B);
                break;
            }
            if(++currentQ > 3) currentQ = 0; // roll next
        break;
        default:
            return;
    }
}

/**
 * @brief ShowScreen - turn on data transmission
 */
void ShowScreen(){
    if(ScrnState == SCREEN_RELAX) ScrnState = SCREEN_UPDATENXT;
}

/**
 * @brief ScreenOFF - turn off screen & clear screen buffer
 */
void ScreenOFF(){
    DBG("Screen off");
    CLEAR(SCLK);
    CLEAR(A);
    CLEAR(B);
    FillScreen(0);
    ScrnState = SCREEN_RELAX;
}

void setdmabuf0(uint8_t pattern, uint8_t N){
    for(int i = 0; i < N; ++i) dmabuf[0][i] = pattern;
}
