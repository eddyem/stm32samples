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

#pragma once

#include <stdint.h>

typedef enum{ // screen states
     SCREEN_INIT        // init stage
    ,SCREEN_W4INIT      // wait after last unsuccessfull update
    ,SCREEN_RELAX       // nothing to do (screen is off)
    ,SCREEN_CONVBUF     // convert next buffer portion
    ,SCREEN_UPDATENXT   // update next block
    ,SCREEN_ACTIVE      // transmission active - wait for SPI transfer ends

} screen_state;

// timeout (ms) in case of SPI error - 100ms
#define SCRN_SPI_TIMEOUT    (100)
// wait if failed on init
#define SCRN_W4INI_TIMEOUT  (1000)

// sprite width and height in pixels
#define SPRITEWD    (8)
#define SPRITEHT    (8)

// maximal font scale
#define FONTSCALEMAX    (10)

// some base colors
#define COLOR_BLACK 0x0000
#define COLOR_BLUE 0x001F
#define COLOR_BROWN 0xA145
#define COLOR_CHOCOLATE 0xD343
#define COLOR_CYAN 0x07FF
#define COLOR_DARKBLUE 0x0011
#define COLOR_DARKCYAN 0x03EF
#define COLOR_DARKGRAY 0x8410
#define COLOR_DARKGREEN 0x03E0
#define COLOR_DARKMAGENTA 0x8811
#define COLOR_DARKORANGE 0xFC60
#define COLOR_DARKRED 0x8800
#define COLOR_DARKVIOLET 0x901A
#define COLOR_DEEPPINK 0xF8B2
#define COLOR_GOLD 0xFEA0
#define COLOR_GRAY 0xAD55
#define COLOR_GREEN 0x07E0
#define COLOR_GREENYELLOW 0xAFE5
#define COLOR_INDIGO 0x4810
#define COLOR_KHAKI 0xF731
#define COLOR_LIGHTBLUE 0xAEDC
#define COLOR_LIGHTCYAN 0xE7FF
#define COLOR_LIGHTGREEN 0x9772
#define COLOR_LIGHTGRAY 0xC618
#define COLOR_LIGHTYELLOW 0xFFFC
#define COLOR_MAGENTA 0xF81F
#define COLOR_MEDIUMBLUE 0x0019
#define COLOR_NAVY 0x000F
#define COLOR_OLIVE 0x7BE0
#define COLOR_ORANGE 0xFD20
#define COLOR_ORANGERED 0xFA20
#define COLOR_PALEGREEN 0x9FD3
#define COLOR_PINK 0xF81F
#define COLOR_PURPLE 0x780F
#define COLOR_RED 0xf800
#define COLOR_SEAGREEN 0x2C4A
#define COLOR_SKYBLUE 0x867D
#define COLOR_SPRINGGREEN 0x07EF
#define COLOR_STEELBLUE 0x4416
#define COLOR_TOMATO 0xFB08
#define COLOR_WHITE 0xFFFF
#define COLOR_YELLOW 0xFFE0
#define COLOR_YELLOWGREEN 0x9E66


screen_state getScreenState();
void ClearScreen();
void UpdateScreen(int y0, int y1);
void setBGcolor(uint16_t c);
void setFGcolor(uint16_t c);
void invertSpriteColor(int xmin, int xmax, int ymin, int ymax);
void DrawPix(int X, int Y, uint8_t pix);
uint8_t SetFontScale(uint8_t scale);
uint8_t DrawCharAt(int X, int Y, uint8_t Char);
int PutStringAt(int X, int Y, const char *str);
int CenterStringAt(int Y, const char *str);
int strpixlen(const char *str);
uint8_t *getScreenBuf();
void process_screen();

