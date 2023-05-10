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

// type for font choosing
typedef enum{
    FONT_T_MIN = -1,    // no fonts <= this
    FONT14,             // 16x16, font height near 14px
    FONTN8,             // numbers and 'A'..'Z', height 8px
    FONT_T_MAX          // no fonts >= this
} font_t;

int choose_font(font_t newfont);
const uint8_t *font_char(uint8_t Char);

typedef struct{
    const uint8_t *font;        // font inself
    const uint8_t *enctable;    // font encoding table
    uint8_t height;             // full font matrix height
    uint8_t bytes;              // amount of bytes in font matrix
    uint8_t baseline;           // baseline position (coordinate from bottom line)
} afont;

extern const afont *curfont;
