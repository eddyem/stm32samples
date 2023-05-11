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

