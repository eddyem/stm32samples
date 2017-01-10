/*
 * keycodes.c
 *
 * Copyright 2015 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 */

#include "keycodes.h"
/*
 * Keyboard buffer:
 * buf[1]: MOD
 * buf[2]: reserved
 * buf[3]..buf[8] - keycodes 1..6
 */
static uint8_t buf[9] = {2,0,0,0,0,0,0,0,0};

#define _(x)  (x|0x80)
// array for keycodes according to ASCII table; MSB is MOD_SHIFT flag
static const uint8_t keycodes[] = {
	// space !"#$%&'
	KEY_SPACE, _(KEY_1), _(KEY_QUOTE), _(KEY_3), _(KEY_4), _(KEY_5), _(KEY_7), KEY_QUOTE,
	// ()*+,-./
	_(KEY_9), _(KEY_0), _(KEY_8), _(KEY_EQUAL), KEY_COMMA, KEY_MINUS, KEY_PERIOD, KEY_SLASH,
	// 0..9
	39, 30, 31, 32, 33, 34, 35, 36, 37, 38,
	// :;<=>?@
	_(KEY_SEMICOLON), KEY_SEMICOLON, _(KEY_COMMA), KEY_EQUAL, _(KEY_PERIOD), _(KEY_SLASH), _(KEY_2),
	// A..Z:  for a in $(seq 0 25); do printf "$((a+132)),"; done
	132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,
	// [\]^_`
	KEY_LEFT_BRACE, KEY_BACKSLASH, KEY_RIGHT_BRACE, _(KEY_6), _(KEY_MINUS), KEY_TILDE,
	// a..z: for a in $(seq 0 25); do printf "$((a+4)),"; done
	4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,
	// {|}~
	_(KEY_LEFT_BRACE), _(KEY_BACKSLASH), _(KEY_RIGHT_BRACE), _(KEY_TILDE)
};

uint8_t *set_key_buf(uint8_t MOD, uint8_t KEY){
	buf[1] = MOD;
	buf[3] = KEY;
	return buf;
}

/**
 * return buffer for sending symbol "ltr" with addition modificator mod
 */
uint8_t *press_key_mod(char ltr, uint8_t mod){
	uint8_t MOD = 0;
	uint8_t KEY = 0;
	if(ltr > 31){
		KEY = keycodes[ltr - 32];
		if(KEY & 0x80){
			MOD = MOD_SHIFT;
			KEY &= 0x7f;
		}
	}else if (ltr == '\n') KEY = KEY_ENTER;
	buf[1] = MOD | mod;
	buf[3] = KEY;
	return buf;
}
