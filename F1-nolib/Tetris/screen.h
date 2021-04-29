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

#pragma once
#ifndef SCREEN_H__
#define SCREEN_H__

#include <stdint.h>

// additional black frames for display won't be so bright
#define NBLACK_FRAMES       (7)

// display size in px
// PANEL_WIDTH is width of one panel
//#define PANEL_WIDTH         (64)
// SCREEN_WIDTH is total screen width
#define SCREEN_WIDTH        (64)
#define SCREEN_HEIGHT       (32)
#define SCREENBUF_SZ        (SCREEN_WIDTH*SCREEN_HEIGHT)
#define SCREEN_HW           (SCREEN_WIDTH/2)
#define SCREEN_HH           (SCREEN_HEIGHT/2)
// amount of blocks @ screen
#define NBLOCKS             (16)

// pause to show a quater of screen - 10ms (25Hz framerate)
#define SCREEN_PAUSE        (3)

typedef enum{ // screen states
    SCREEN_RELAX,       // nothing to do (screen is off)
    SCREEN_ACTIVE,      // transmission active
    SCREEN_UPDATENXT    // update next quarter
} screen_state;

// colors
#define COLOR_BLACK     (0b00000000)
#define COLOR_WHITE     (0b11111111)
#define COLOR_BLUE      (0b00000011)
#define COLOR_LBLUE     (0b00000001)
#define COLOR_GREEN     (0b00011100)
#define COLOR_LGREEN    (0b00001100)
#define COLOR_RED       (0b11100000)
#define COLOR_LRED      (0b01100000)
#define COLOR_CYAN      (0b00011111)
#define COLOR_LCYAN     (0b00000101)
#define COLOR_PURPLE    (0b11100011)
#define COLOR_YELLOW    (0b11111100)
#define COLOR_LYELLOW   (0b00100100)

extern uint32_t score; // current game score
extern uint8_t screenbuf[SCREENBUF_SZ];
extern uint32_t StepMS, Tlast, incSpd; // common for all games timer values

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
uint8_t CenterStringAt(int16_t Y, const char *str);
int strpixlen(const char *str);
uint8_t *getScreenBuf();
void process_screen();
void ScreenON();
void ScreenOFF();
uint32_t getFPS();

uint8_t RGB2pack(const uint8_t *RGB);
uint8_t *pack2RGB(uint8_t pack);


#endif // SCREEN_H__
