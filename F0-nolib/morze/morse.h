/*
 *                                                                                                  geany_encoding=koi8-r
 * morse.h
 *
 * Copyright 2017 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#pragma once
#ifndef __MORSE_H__
#define __MORSE_H__
#include <stdint.h>

/*
International Morse code is composed of five elements:

    short mark, dot or "dit" (Œ): "dot duration" is one time unit long == 50ms
    longer mark, dash or "dah" (ŒŒŒ): three time units long == 150ms
    inter-element gap between the dots and dashes within a character: one dot duration or one unit long == 50ms
    short gap (between letters): three time units long == 150ms
    medium gap (between words): seven time units long == 350ms
*/
// base timing (in ms)
#define DOT_LEN     (75)
#define DASH_LEN    (3*DOT_LEN)
#define SHRT_GAP    (DOT_LEN)
// minus shr gap
#define LTR_GAP     (2*DOT_LEN)
// minus ltr gap
#define WRD_GAP     (4*DOT_LEN)

extern uint16_t mbuff[];
char *fillbuffer(char *ltr, uint8_t *len);


#endif // __MORSE_H__
