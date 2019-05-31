/*
 *                                                                                                  geany_encoding=koi8-r
 * morse.c
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
#include "stm32f0.h"
#include "morse.h"

#define MAXPINGS (8)

uint16_t mbuff[3*MAXPINGS];
/*
32 (0x20) -      64 (0x40) - @   96 (0x60) - `  128 (0x80) - Ä  160 (0xa0) - †  192 (0xc0) - ¿  224 (0xe0) - ‡
 33 (0x21) - !   65 (0x41) - A   97 (0x61) - a  129 (0x81) - Å  161 (0xa1) - °  193 (0xc1) - ¡  225 (0xe1) - ·
 34 (0x22) - "   66 (0x42) - B   98 (0x62) - b  130 (0x82) - Ç  162 (0xa2) - ¢  194 (0xc2) - ¬  226 (0xe2) - ‚
 35 (0x23) - #   67 (0x43) - C   99 (0x63) - c  131 (0x83) - É  163 (0xa3) - £  195 (0xc3) - √  227 (0xe3) - „
 36 (0x24) - $   68 (0x44) - D  100 (0x64) - d  132 (0x84) - Ñ  164 (0xa4) - §  196 (0xc4) - ƒ  228 (0xe4) - ‰
 37 (0x25) - %   69 (0x45) - E  101 (0x65) - e  133 (0x85) - Ö  165 (0xa5) - •  197 (0xc5) - ≈  229 (0xe5) - Â
 38 (0x26) - &   70 (0x46) - F  102 (0x66) - f  134 (0x86) - Ü  166 (0xa6) - ¶  198 (0xc6) - ∆  230 (0xe6) - Ê
 39 (0x27) - '   71 (0x47) - G  103 (0x67) - g  135 (0x87) - á  167 (0xa7) - ß  199 (0xc7) - «  231 (0xe7) - Á
 40 (0x28) - (   72 (0x48) - H  104 (0x68) - h  136 (0x88) - à  168 (0xa8) - ®  200 (0xc8) - »  232 (0xe8) - Ë
 41 (0x29) - )   73 (0x49) - I  105 (0x69) - i  137 (0x89) - â  169 (0xa9) - ©  201 (0xc9) - …  233 (0xe9) - È
 42 (0x2a) - *   74 (0x4a) - J  106 (0x6a) - j  138 (0x8a) - ä  170 (0xaa) - ™  202 (0xca) -    234 (0xea) - Í
 43 (0x2b) - +   75 (0x4b) - K  107 (0x6b) - k  139 (0x8b) - ã  171 (0xab) - ´  203 (0xcb) - À  235 (0xeb) - Î
 44 (0x2c) - ,   76 (0x4c) - L  108 (0x6c) - l  140 (0x8c) - å  172 (0xac) - ¨  204 (0xcc) - Ã  236 (0xec) - Ï
 45 (0x2d) - -   77 (0x4d) - M  109 (0x6d) - m  141 (0x8d) - ç  173 (0xad) - ≠  205 (0xcd) - Õ  237 (0xed) - Ì
 46 (0x2e) - .   78 (0x4e) - N  110 (0x6e) - n  142 (0x8e) - é  174 (0xae) - Æ  206 (0xce) - Œ  238 (0xee) - Ó
 47 (0x2f) - /   79 (0x4f) - O  111 (0x6f) - o  143 (0x8f) - è  175 (0xaf) - Ø  207 (0xcf) - œ  239 (0xef) - Ô
 48 (0x30) - 0   80 (0x50) - P  112 (0x70) - p  144 (0x90) - ê  176 (0xb0) - ∞  208 (0xd0) - –  240 (0xf0) - 
 49 (0x31) - 1   81 (0x51) - Q  113 (0x71) - q  145 (0x91) - ë  177 (0xb1) - ±  209 (0xd1) - —  241 (0xf1) - Ò
 50 (0x32) - 2   82 (0x52) - R  114 (0x72) - r  146 (0x92) - í  178 (0xb2) - ≤  210 (0xd2) - “  242 (0xf2) - Ú
 51 (0x33) - 3   83 (0x53) - S  115 (0x73) - s  147 (0x93) - ì  179 (0xb3) - ≥  211 (0xd3) - ”  243 (0xf3) - Û
 52 (0x34) - 4   84 (0x54) - T  116 (0x74) - t  148 (0x94) - î  180 (0xb4) - ¥  212 (0xd4) - ‘  244 (0xf4) - Ù
 53 (0x35) - 5   85 (0x55) - U  117 (0x75) - u  149 (0x95) - ï  181 (0xb5) - µ  213 (0xd5) - ’  245 (0xf5) - ı
 54 (0x36) - 6   86 (0x56) - V  118 (0x76) - v  150 (0x96) - ñ  182 (0xb6) - ∂  214 (0xd6) - ÷  246 (0xf6) - ˆ
 55 (0x37) - 7   87 (0x57) - W  119 (0x77) - w  151 (0x97) - ó  183 (0xb7) - ∑  215 (0xd7) - ◊  247 (0xf7) - ˜
 56 (0x38) - 8   88 (0x58) - X  120 (0x78) - x  152 (0x98) - ò  184 (0xb8) - ∏  216 (0xd8) - ÿ  248 (0xf8) - ¯
 57 (0x39) - 9   89 (0x59) - Y  121 (0x79) - y  153 (0x99) - ô  185 (0xb9) - π  217 (0xd9) - Ÿ  249 (0xf9) - ˘
 58 (0x3a) - :   90 (0x5a) - Z  122 (0x7a) - z  154 (0x9a) -    186 (0xba) - ∫  218 (0xda) - ⁄  250 (0xfa) - ˙
 59 (0x3b) - ;   91 (0x5b) - [  123 (0x7b) - {  155 (0x9b) - õ  187 (0xbb) - ª  219 (0xdb) - €  251 (0xfb) - ˚
 60 (0x3c) - <   92 (0x5c) - \  124 (0x7c) - |  156 (0x9c) - ú  188 (0xbc) - º  220 (0xdc) - ‹  252 (0xfc) - ¸
 61 (0x3d) - =   93 (0x5d) - ]  125 (0x7d) - }  157 (0x9d) - ù  189 (0xbd) - Ω  221 (0xdd) - ›  253 (0xfd) - ˝
 62 (0x3e) - >   94 (0x5e) - ^  126 (0x7e) - ~  158 (0x9e) - û  190 (0xbe) - æ  222 (0xde) - ﬁ  254 (0xfe) - ˛
 63 (0x3f) - ?   95 (0x5f) - _  127 (0x7f) -    159 (0x9f) - ü  191 (0xbf) - ø  223 (0xdf) - ﬂ  255 (0xff) - ˇ
*/
// 1 - len in pings, 2 - pings (0 - dot, 1 - dash), reverse order
static const uint8_t ascii[] = { // starting from space+1
    6, 53, // ! -*-*--
    6, 18, // " *-**-*
    5, 29, // # [no] -*---
    7, 72, // $ ***-**-
    8, 86, // % [pc] *--*-*-*
    5, 2,  // & *-***
    6, 30, // ' *----*
    5, 13, // ( -*--*
    6, 45, // ) -*--*-
    7, 40, // * [str] ***-*-*
    5, 10, // + *-*-*
    6, 51, // , --**--
    6, 33, // - -****-
    6, 42, // . *-*-*-
    5, 9,  // / -**-*
    5, 31, // 0 -----
    5, 30, // 1 *----
    5, 28, // 2 **---
    5, 24, // 3 ***--
    5, 16, // 4 ****-
    5, 0,  // 5 *****
    5, 1,  // 6 -****
    5, 3,  // 7 --***
    5, 7,  // 8 ---**
    5, 15, // 9 ----*
    6, 7,  // : ---***
    6, 21, // ; -*-*-*
    7, 2,  // < [ls] *-*****
    5, 17, // = -***-
    6, 3,  // > [gs] --****
    6, 12, // ? **--**
    6, 22, // @ *--*-*
    2, 2,  // a  *-
    4, 1,  // b -***
    4, 5,  // c -*-*
    3, 1,  // d -**
    1, 0,  // e *
    4, 4,  // f **-*
    3, 3,  // g --*
    4, 0,  // h ****
    2, 0,  // i **
    4, 14, // j *---
    3, 5,  // k -*-
    4, 2,  // l *-**
    2, 3,  // m --
    2, 1,  // n -*
    3, 7,  // o ---
    4, 6,  // p *--*
    4, 11, // q --*-
    3, 2,  // r *-*
    3, 0,  // s ***
    1, 1,  // t -
    3, 4,  // u **-
    4, 8,  // v ***-
    3, 6,  // w *--
    4, 9,  // x -**-
    4, 13, // y -*--
    4, 3,  // z --**
    5, 13, // [ [(]
    6, 40, // \ [End of work] ***-*- //
    6, 45, // ] [)]
    7, 96, // ^ [hat] *****--
    6, 44, // _ **--*-
    6, 30, // ` [']
    2, 2,  // a  *-
    4, 1,  // b -***
    4, 5,  // c -*-*
    3, 1,  // d -**
    1, 0,  // e *
    4, 4,  // f **-*
    3, 3,  // g --*
    4, 0,  // h ****
    2, 0,  // i **
    4, 14, // j *---
    3, 5,  // k -*-
    4, 2,  // l *-**
    2, 3,  // m --
    2, 1,  // n -*
    3, 7,  // o ---
    4, 6,  // p *--*
    4, 11, // q --*-
    3, 2,  // r *-*
    3, 0,  // s ***
    1, 1,  // t -
    3, 4,  // u **-
    4, 8,  // v ***-
    3, 6,  // w *--
    4, 9,  // x -**-
    4, 13, // y -*--
    4, 3,  // z --**
    5, 13, // { [(]
    6, 30, // | [']
    6, 45, // } [)]
    8, 37, // ~ [tld] -*-**-**
};
/*
 * I was too lazy
static const uint8_t koi8[] = {

};*/

/**
 * return next character in word
 * @param str (i) - string to send
 * @param len (o) - length of buffer/3
 */
char *fillbuffer(char *str, uint8_t *len){
    const uint8_t *arr = NULL;
    char ltr = *str++;
    if(ltr > ' ' && ltr < 127){ arr = ascii; ltr -= '!'; };
    if(!arr){*len = 1; mbuff[0] = WRD_GAP+DASH_LEN; mbuff[2] = 0; return str;};
    ltr *= 2;
    uint8_t i, N = arr[(uint8_t)ltr], code = arr[(uint8_t)ltr+1];
    uint16_t *ptr = mbuff;
    for(i = 0; i < N; ++i){
        uint16_t L = DOT_LEN;
        if(code & 1) L = DASH_LEN;
        *ptr++ = L + SHRT_GAP; *ptr++ = 0; *ptr++ = L;
        code >>= 1;
    }
    ptr[-3] += LTR_GAP;
    *len = N;
    return str;
}
