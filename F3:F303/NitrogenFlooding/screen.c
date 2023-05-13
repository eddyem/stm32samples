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

#include <string.h> // memset, memcpy
#include <stdio.h>
#include "fonts.h"
#include "hardware.h"
#include "screen.h"
#include "spi.h" // spi_status
#include "ili9341.h"
#include "usb.h"
#ifdef EBUG
#include "strfunc.h"
#endif

#include <string.h>

#define SWAPINT(a,b) do{register int r = a; a = b; b = r;}while(0)

// sprites 8x8 bytes with 8bit foreground and 8bit background

// X coordinate - from left to right!
// Y coordinate - from top to bottom!
// (0,0) is top left corner


// full screen buffer size in bytes/words (depending on sprite size)
// !!! If sprite width != 8, change all code !!!
#define SCREENBUF_SZ (SCRNSZ/SPRITEWD)
// amount of sprites by both axes
#define SCRNSPRITEW (SCRNW/SPRITEWD)
#define SCRNSPRITEH (SCRNH/SPRITEHT)
#define SPRITE_SZ (SCRNSPRITEW * SCRNSPRITEH)

// pixel buffer
static uint8_t screenbuf[SCREENBUF_SZ];
// color buffers
static uint16_t foreground[SPRITE_SZ];
static uint16_t background[SPRITE_SZ];
// borders of string numbers to update screen (including!)
static int uy0, uy1;
// index of pixel in given updating block
static int updidx = 0; // ==-1 to initialize update
// next data portion size (in bytes!), total amount of bytes in update buffer
static int portionsz = 0, updbuffsz;
// font scale
uint8_t fontscale = 1;
int fontheight = 0, fontbase = 0;

static uint16_t fgColor = 0xff, bgColor = 0; // foreground and background colors
void setBGcolor(uint16_t c){bgColor = c;}
void setFGcolor(uint16_t c){fgColor = c;}

screen_state ScrnState = SCREEN_INIT;

/**
 * @brief UpdateScreen - request to screen updating from lines y0 to y1 (including)
 * @param y0, y1 - first and last line of field to update
 */
void UpdateScreen(int y0, int y1){
    //if(y0 > y1) SWAPINT(y0, y1);
    if(y0 < 0) y0 = 0;
    if(y1 > SCRNH || y1 < 0) y1 = SCRNH - 1;
    uy0 = y0; uy1 = y1;
    updidx = -1;
    updbuffsz = SCRNW * (1 + y1 - y0);
}

/**
 * @brief FillScreen - fill screen buffer with current bgColor
 */
void ClearScreen(){
    memset(screenbuf, 0, SCREENBUF_SZ);
    for(int i = 0; i < SPRITE_SZ; ++i){
        foreground[i] = fgColor;
        background[i] = bgColor;
    }
    UpdateScreen(0, SCRNH-1);
}

#define DRAWPIX(X, Y, pix) \
    if((X) > -1 && (X) < SCRNW && (Y) > -1 && (Y) < SCRNH){ \
        int16_t spritex = (X)/SPRITEWD, spriteidx = spritex + SCRNSPRITEW * ((Y) / SPRITEHT); \
        uint8_t *ptr = &screenbuf[(Y)*SCRNSPRITEW + spritex]; \
        if(pix) *ptr |= 1 << (7 - ((X)%8)); \
        else *ptr &= ~(1 << (7 - ((X)%8))); \
        foreground[spriteidx] = fgColor; \
        background[spriteidx] = bgColor; \
    }

/**
 * @brief DrawPix - set or clear pixel
 * @param X, Y - pixel coordinates (could be outside of screen)
 * @param pix - 1 - foreground, 0 - background
 */
void DrawPix(int X, int Y, uint8_t pix){
    DRAWPIX(X, Y, pix);
}

/**
 * @brief invertSpriteColor - change bg-fg colors in given rectangle
 * @param xmin, xmax, ymin, ymax - rectangle coordinates
 */
void invertSpriteColor(int xmin, int xmax, int ymin, int ymax){
    if(xmin < 0) xmin = 0;
    if(xmax > SCRNW-1) xmax = SCRNW-1;
    if(ymin < 0) ymin = 0;
    if(ymax > SCRNH-1) ymax = SCRNH-1;
    if(xmin > xmax) SWAPINT(xmin, xmax);
    if(ymin > ymax) SWAPINT(ymin, ymax);
    // convert to sprite coordinates
    xmin /= SPRITEWD; xmax = (xmax + SPRITEWD - 1) / SPRITEWD;
    ymin /= SPRITEHT; ymax = (ymax + SPRITEHT - 1) / SPRITEHT;
    for(int y = ymin; y <= ymax; ++y){
        int idx = y * SCRNSPRITEW + xmin;
        uint16_t *f = foreground + idx, *b = background + idx;
        for(int x = xmin; x <= xmax; ++x, ++f, ++b){
            register uint16_t r = *f;
            *f = *b; *b = r;
        }
    }
}

uint8_t SetFontScale(uint8_t scale){
    if(scale > 0 && scale <= FONTSCALEMAX) fontscale = scale;
    fontheight = curfont->height * fontscale;
    fontbase = curfont->baseline * fontscale;
    return fontscale;
}

// left and right bits clearing masks
//static const uint8_t lmask[8] = {0xff, 0b01111111, 0b00111111, 0b00011111, 0b00001111, 0b00000111, 0b00000011, 0b00000001};
//static const uint8_t rmask[8] = {0, 0b10000000, 0b11000000, 0b11100000, 0b11110000, 0b11111000, 0b11111100, 0b11111110};

// TODO in case of low speed: draw at once full line?
/**
 * @brief DrawCharAt - draws character @ position X,Y (this point is left baseline corner of char!)
 * character will be drawn with current fg and bg colors
 * @param X, Y  - started point
 * @param Char - char to draw
 * @return char width
 */
uint8_t DrawCharAt(int X, int Y, uint8_t Char){
    const uint8_t *curchar = font_char(Char);
    if(!curchar) return 0;
    // now change Y coordinate to left upper corner of font
    Y += fontbase - fontheight + 1;
    // height and width of letter in pixels
    uint8_t h = curfont->height, w = *curchar++; // now curchar is pointer to bits array
    uint8_t lw = curfont->bytes / h; // width of letter in bytes
    for(uint8_t row = 0; row < h; ++row){
        int Y1 = Y + fontscale * row;
        for(uint8_t col = 0; col < w; ++col){
            register uint8_t pix = curchar[row*lw + (col/8)] & (1 << (7 - (col%8)));
            int xx = X + fontscale * col;
            for(int y = 0; y < fontscale; ++y) for(int x = 0; x < fontscale; ++x){
                int nxtx = xx+x, nxty = Y1+y;
                DRAWPIX(nxtx, nxty, pix);
            }
        }
    }
    return w * fontscale;
}

/**
 * @brief PutStringAt - draw text string @ screen
 * @param X, Y - base coordinates
 * @param str  - string to draw
 * @return - X-coordinate of last pixel
 */
int PutStringAt(int X, int Y, const char *str){
    if(!str) return 0;
    while(*str){
        X += DrawCharAt(X, Y, (uint8_t)*str++);
    }
    return X;
}

/**
 * @brief CenterStringAt - draw string @ center line
 * @param Y   - y coordinate
 * @param str - string
 * @return - text width in pixels
 */
int CenterStringAt(int Y, const char *str){
    int l = strpixlen(str);
    return PutStringAt((SCRNW - l)/2, Y, str);
}

// full string length in pixels
int strpixlen(const char *str){
    if(!str) return 0;
    int l = 0;
    while(*str){
        const uint8_t *c = font_char(*str++);
        if(c) l += *c;
    }
    return l * fontscale;
}

// convert buffer  to update (return 0 if all sent)
static int convbuf(){
//    DBG("convert buffer");
    int rest = updbuffsz - updidx;
/*   USB_sendstr("updbuffsz="); USB_sendstr(i2str(updbuffsz));
   USB_sendstr("\nupdidx="); USB_sendstr(i2str(updidx));
   USB_sendstr("\nrest="); USB_sendstr(i2str(rest)); newline();*/
    if(rest < 1) return 0;
    if(rest > COLORBUFSZ) rest = COLORBUFSZ;
    portionsz = rest;
//   USB_sendstr("portionsz="); USB_sendstr(i2str(portionsz)); newline();
    int Y = uy0 + updidx / SCRNW; // starting Y of updating string
    uint16_t *o = colorbuf; // output color data
    uint8_t *i = screenbuf + (Y*SCRNSPRITEW); // starting portion of pixel info
    while(rest > 0){
        int spidx = (Y/SPRITEHT)*SCRNSPRITEW; // index in color array
        uint16_t *fg = foreground + spidx, *bg = background + spidx;
        for(int X = 0; X < SCRNSPRITEW; ++X, ++fg, ++bg, ++i){
            // prepare colors for SPI transfer
            uint16_t f = __builtin_bswap16(*fg), b = __builtin_bswap16(*bg);
            uint8_t pix = *i;
            for(int idx = 0; idx < SPRITEWD; ++idx){ // now check bits in pixels mask
                *o++ = (pix & 0x80) ? f : b;
                pix <<= 1;
                ++updidx; --rest;
            }
        }
        ++Y;
    }
    return 1;
}

// check SPI timeout
static uint32_t Tscr_last = 0;
static int chk_tmout(){
    if(Tms - Tscr_last > SCRN_SPI_TIMEOUT){
        ScrnState = SCREEN_INIT;
        UpdateScreen(0, SCRNH-1);
        return 1;
    }
    return 0;
}
/**
 * @brief process_screen - screen state machine processing
 */
void process_screen(){
    switch(ScrnState){
        case SCREEN_INIT: // try to init SPI and screen
            DBG("SCREEN_INIT");
            spi_setup();
            choose_font(FONT14);
            SetFontScale(1);
            if(ili9341_init()) ScrnState = SCREEN_RELAX;
            else{
                Tscr_last = Tms;
                ScrnState = SCREEN_W4INIT;
            }
        break;
        case SCREEN_W4INIT:
            if(Tms - Tscr_last > SCRN_W4INI_TIMEOUT) ScrnState = SCREEN_INIT;
        break;
        case SCREEN_RELAX: // check need of updating
            if(updidx > -1) return;
            //DBG("Need to update");
            if(!ili9341_setcol(0, SCRNW-1)) return;
            if(!ili9341_setrow(uy0, uy1)) return;
            if(!ili9341_writecmd(ILI9341_RAMWR)) return;
            updidx = 0;
            Tscr_last = Tms;
            ScrnState = SCREEN_ACTIVE; // now we are ready to update screen
        // fallthrough
        case SCREEN_ACTIVE: // SPI transmission active
            if(chk_tmout()){
                DBG("timeout");
                return;
            }
            if(spi_status != SPI_READY){
                DBG("SPI not ready");
                return;
            }
            // convert & update next field
            ScrnState = SCREEN_CONVBUF;
        // fallthrough
        case SCREEN_CONVBUF: // convert buffer for sending
            if(!convbuf()){
                ScrnState = SCREEN_RELAX;
                return;
            }
            Tscr_last = Tms; // for DMA writing timeout
            ScrnState = SCREEN_UPDATENXT;
        // fallthrough
        case SCREEN_UPDATENXT: // send next data portion
            if(chk_tmout()){
                DBG("timeout");
                return;
            }
            // portionsz in pixels (uint16_t), sending size in bytes!
            if(!ili9341_writedata((uint8_t*)colorbuf, portionsz * 2)) return;
            Tscr_last = Tms; // wait DMA writing timeout
            ScrnState = SCREEN_ACTIVE;
        break;
        default:
        break;
    }
}
