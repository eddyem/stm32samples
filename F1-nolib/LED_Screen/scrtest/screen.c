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
#include "screen.h"

// !!!FOR LITTLE-ENDIAN!!!

// X coordinate - from left to right!
// Y coordinate - from top to bottom!
// (0,0) is top left corner

// all-screen buffer
static uint8_t screenbuf[SCREENBUF_SZ];
// buffers for DMA - for each of four parts
static uint8_t dmabuf[4][SCREENBUF_SZ/4];

/**
 * @brief FillScreen - fill screen buffer with 0 or 1
 * @param setclear   - !=1 to set & ==0 to reset
 */
void FillScreen(uint8_t setclear){
    uint8_t pattern = 0;
    if(setclear) pattern = 0xff;
    memset(screenbuf, pattern, SCREENBUF_SZ);
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
    for(uint8_t row = 0; row <= h; ++row){
        for(uint8_t col = 0; col < w; ++col){
            DrawPix(X + col, Y + row, curchar[row*lw + (col/8)] & (1 << (7 - (col%8)))); // only for little-endian
        }
    }
    return w;
}

/**
 * @brief ConvertScreenBuf - convert scscreenbuf into dmabuf
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
uint8_t PutStringAt(int16_t X, int16_t Y, char *str){
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
