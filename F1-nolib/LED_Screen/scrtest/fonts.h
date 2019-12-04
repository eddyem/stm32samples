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
#ifndef FONTS_H__
#define FONTS_H__

#include <stdint.h>

// code number of first symbol in font table
#define FIRST_SYMBOL_CODE 32
// total amount of symbols - all without first 32
#define SYMBOLS_AMOUNT      (256-FIRST_SYMBOL_CODE)

// type for font choosing
typedef enum{
    FONT_T_MIN = -1,    // no fonts <= this
    FONT14,             // 16x16, font height near 14px
    FONT16,             // 16x16, font height 16px
    FONT_T_MAX          // no fonts >= this
} font_t;

int choose_font(font_t newfont);
/* uint8_t fontheight();
uint8_t fontbaseline();
uint8_t fontbytes(); */
const uint8_t *font_char(uint8_t Char);


typedef struct{
    const uint8_t *font;// font inself
    uint8_t height;     // full font matrix height
    uint8_t bytes;      // amount of bytes in font matrix
    uint8_t baseline;   // baseline position (coordinate from bottom line)
} afont;

extern const afont *curfont;

#endif // FONTS_H__
