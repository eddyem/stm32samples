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

#pragma once
#ifndef SCREEN_H__
#define SCREEN_H__

#include <stdint.h>

// display size in px
// PANEL_WIDTH is width of one panel
//#define PANEL_WIDTH         (64)
// SCREEN_WIDTH is total screen width
#define SCREEN_WIDTH        (64)
#define SCREEN_HEIGHT       (32)
#define SCREENBUF_SZ        (SCREEN_WIDTH*SCREEN_HEIGHT)
// amount of blocks @ screen
#define NBLOCKS             (16)

// pause to show a quater of screen - 10ms (25Hz framerate)
#define SCREEN_PAUSE        (3)

typedef enum{ // screen states
    SCREEN_RELAX,       // nothing to do (screen is off)
    SCREEN_ACTIVE,      // transmission active
    SCREEN_UPDATENXT    // update next quarter
} screen_state;

screen_state getScreenState();
void ClearScreen();
void setBGcolor(uint8_t c);
void setFGcolor(uint8_t c);
void DrawPix(int16_t X, int16_t Y, uint8_t pix);
void InvertPix(int16_t X, int16_t Y);
void XORPix(int16_t X, int16_t Y, uint8_t pix);
uint8_t GetPix(int16_t X, int16_t Y);
uint8_t DrawCharAt(int16_t X, int16_t Y, uint8_t Char);
uint8_t PutStringAt(int16_t X, int16_t Y, const char *str);
uint8_t *getScreenBuf();
void process_screen();
void ScreenON();
void ScreenOFF();
uint32_t getFPS();

uint8_t RGB2pack(const uint8_t *RGB);
uint8_t *pack2RGB(uint8_t pack);


#endif // SCREEN_H__
