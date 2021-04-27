/*
 * This file is part of the TETRIS project.
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
#include "debug.h"
#include "fonts.h"
#include "hardware.h"
#include "screen.h"


// !!!FOR LITTLE-ENDIAN!!!

uint32_t score = 0;  // current game's score
uint32_t StepMS, Tlast, incSpd = 0; // common for all games timer values

// X coordinate - from left to right!
// Y coordinate - from top to bottom!
// (0,0) is top left corner

// all-screen buffer
uint8_t screenbuf[SCREENBUF_SZ];
extern volatile uint32_t Tms; // time for FPS count
static uint32_t FPS = 0, Tfps = 0; // approx FPS
uint32_t getFPS(){return FPS;}

static uint8_t fgColor = 0xff, bgColor = 0; // foreground and background colors
void setBGcolor(uint8_t c){bgColor = c;}
void setFGcolor(uint8_t c){fgColor = c;}

/**
 * @brief FillScreen - fill screen buffer with current bgColor
 */
void ClearScreen(){
    for(int i = 0; i < SCREENBUF_SZ; ++i)
        screenbuf[i] = bgColor;
}

/**
 * @brief DrawPix - set or clear pixel
 * @param X, Y - pixel coordinates (could be outside of screen)
 * @param pix - == pixel color (RRRGGGBB)
 */
void DrawPix(int16_t X, int16_t Y, uint8_t pix){
    if(X < 0 || X > SCREEN_WIDTH-1 || Y < 0 || Y > SCREEN_HEIGHT-1) return; // outside of screen
    // now calculate coordinate of pixel
    screenbuf[Y*SCREEN_WIDTH + X] = pix;
}
void InvertPix(int16_t X, int16_t Y){
    if(X < 0 || X > SCREEN_WIDTH-1 || Y < 0 || Y > SCREEN_HEIGHT-1) return; // outside of screen
    screenbuf[Y*SCREEN_WIDTH + X] = ~screenbuf[Y*SCREEN_WIDTH + X];
}
void XORPix(int16_t X, int16_t Y, uint8_t pix){
    if(X < 0 || X > SCREEN_WIDTH-1 || Y < 0 || Y > SCREEN_HEIGHT-1) return; // outside of screen
    screenbuf[Y*SCREEN_WIDTH + X] ^= pix;
}
uint8_t GetPix(int16_t X, int16_t Y){
    if(X < 0 || X > SCREEN_WIDTH-1 || Y < 0 || Y > SCREEN_HEIGHT-1) return 0;
    return screenbuf[Y*SCREEN_WIDTH + X];
}

/**
 * @brief DrawCharAt - draws character @ position X,Y (this point is left baseline corner of char!)
 * character will be drawn with current fg and bg colors
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
            if(curchar[row*lw + (col/8)] & (1 << (7 - (col%8))))
                DrawPix(X + col, Y + row, fgColor);
            else
                DrawPix(X + col, Y + row, bgColor);
        }
    }
    return w;
}

/**
 * @brief PutStringAt - draw text string @ screen
 * @param X, Y - base coordinates
 * @param str  - string to draw
 * @return - text width in pixels
 */
uint8_t PutStringAt(int16_t X, int16_t Y, const char *str){
    DBG("PutStringAt("); DBGU(X); DBG(", "); DBGU(Y); DBG(", \""); DBG(str); DBG("\"\n");
    if(!str) return 0;
    int16_t Xold = X;
    while(*str){
        X += DrawCharAt(X, Y, (uint8_t)*str++);
    }
    return X - Xold;
}

/**
 * @brief CenterStringAt - draw string @ center line
 * @param Y   - y coordinate
 * @param str - string
 * @return - text width in pixels
 */
uint8_t CenterStringAt(int16_t Y, const char *str){
    int l = strpixlen(str);
    return PutStringAt((SCREEN_WIDTH - l)/2, Y, str);
}

// full string length in pixels
int strpixlen(const char *str){
    int l = 0;
    while(*str){
        const uint8_t *c = font_char(*str++);
        if(c) l += *c;
    }
    return l;
}

static screen_state ScrnState = SCREEN_RELAX;
screen_state getScreenState(){return ScrnState;}

static uint8_t currentB = 0; // current block number
static uint8_t Ntick = 0; // tick number (0..7) for PWM color

/**
 * @brief process_screen - screen state machine processing
 */
void process_screen(){
    static uint32_t framecnt = 0;
    switch(ScrnState){
        case SCREEN_ACTIVE: // transmission active
            if(transfer_done){
                ScrnState = SCREEN_UPDATENXT;
            }else return;
        // fallthrough
        case SCREEN_UPDATENXT:
            ConvertScreenBuf(screenbuf, currentB, Ntick); // convert data
            TIM_DMA_transfer(currentB); // start transfer
            ScrnState = SCREEN_ACTIVE;
            if(++Ntick >= 7){
                Ntick = 0;
                if(++currentB >= NBLOCKS){
                    currentB = 0; // start again
                    ++framecnt;
                    if(Tms - Tfps > 999){
                        FPS = framecnt;
                        Tfps = Tms;
                        framecnt = 0;
                    }
                }
            }
        break;
        default:
            return;
    }
}

/**
 * @brief ShowScreen - turn on data transmission
 */
void ScreenON(){
    ScrnState = SCREEN_UPDATENXT;
    Tfps = Tms;
    process_screen();
}

void ScreenOFF(){
    stopTIMDMA();
    SCRN_DISBL();
    ScrnState = SCREEN_RELAX;
    currentB = 0;
    Ntick = 0;
}

/**
 * @brief RGB2pack - convert RGB color to packed RRRGGGBB
 * @param RGB - color (each in range 0..8)
 * @return packed color
 */
uint8_t RGB2pack(const uint8_t *RGB){
    uint8_t b = RGB[2] & 7;
    if(b) b = (b - 1) >> 1;
    return ((RGB[0] & 7)<<5 | (RGB[1] & 7) << 2 | b);
}
/**
 * @brief pack2RGB - convert packed color to array of RGB (0..8)
 * @param pack - packed color
 * @return  RGB array
 */
uint8_t *pack2RGB(uint8_t pack){
    static uint8_t RGB[3];
    RGB[0] = pack >> 5;
    RGB[1] = (pack & 0x1c) >> 2;
    pack &= 3;
    if(pack) RGB[2] = (pack<<1) + 1;
    else RGB[2] = 0;
    return RGB;
}

