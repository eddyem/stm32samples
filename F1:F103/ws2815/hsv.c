/*
 * This file is part of the ws2815 project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "hsv.h"

/**
 * @brief hsv2grb - convert HSV to 24bit GRB
 * @param h (0..359) - Hue in degrees
 * @param s (0..100) - Saturation (color depth)
 * @param v (0..100) - Value (intensity)
 * @return 24-bit  color
 */
uint32_t hsv2grb(uint16_t h, uint8_t s, uint8_t v){
    if(h > 359) h %= 360;
    if(s > 100) s %= 101;
    if(v > 100) v %= 101;
    int Hi = (h/60) % 6;
    int Vmin = (100 - s) * v;
    Vmin /= 100;
    int a = (v - Vmin) * (h % 60);
    a /= 60;
    int Vinc = Vmin + a;
    int Vdec = v - a;
    int r, g, b;
    switch(Hi){
        case 1:
            r = Vdec; g = v; b = Vmin;
        break;
        case 2:
            r = Vmin; g = v; b = Vinc;
        break;
        case 3:
            r = Vmin; g = Vdec; b = v;
        break;
        case 4:
            r = Vinc; g = Vmin; b = v;
        break;
        case 5:
            r = v; g = Vmin; b = Vdec;
        break;
        default:
            r = v; g = Vinc; b = Vmin;
    }
    // now convert r/g/b from percents to 0..255
    r = (r*255)/100;
    g = (g*255)/100;
    b = (b*255)/100;
    return ((b<<16) | (r<<8) | g); // little endian!!!
}
