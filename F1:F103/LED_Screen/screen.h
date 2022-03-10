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
#define PANEL_WIDTH         32
// SCREEN_WIDTH is total screen width
#define SCREEN_WIDTH        64
#define SCREEN_HEIGHT       16
#define SCREENBUF_SZ        (SCREEN_WIDTH*SCREEN_HEIGHT/8)
#define DMABUF_SZ           (SCREENBUF_SZ/4)

// pause to show a quater of screen - 10ms (25Hz framerate)
#define SCREEN_PAUSE        3

// screen is positive (1->on, 0->off)
#define SCREEN_IS_NEGATIVE  1

void FillScreen(uint8_t setclear);
void DrawPix(int16_t X, int16_t Y, uint8_t pix);
uint8_t DrawCharAt(int16_t X, int16_t Y, uint8_t Char);
void ConvertScreenBuf();
uint8_t PutStringAt(int16_t X, int16_t Y, char *str);
uint8_t *getScreenBuf();
uint8_t *getDmaBuf(uint8_t N);
void process_screen();
void ShowScreen();
void ScreenOFF();

void setdmabuf0(uint8_t pattern, uint8_t N);

#endif // SCREEN_H__
