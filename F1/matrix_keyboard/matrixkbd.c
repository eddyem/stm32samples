/*
 * matrixkbd.c
 * Simple utilite for working with matrix keyboard 3columns x 4rows
 * Don't understand multiple keys!!!
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

#include "matrixkbd.h"

extern volatile uint32_t Timer;

#if !defined(KBD_3BY4) && !defined(KBD_4BY4)
	#error You should define keyboard type: KBD_3BY4 or KBD_4BY4
#endif

/*
 * Rows are inputs, columns are outputs
 */
#define ROWS 4
const uint32_t row_ports[ROWS] = {ROW1_PORT, ROW2_PORT, ROW3_PORT, ROW4_PORT};
const uint16_t row_pins[ROWS] = {ROW1_PIN, ROW2_PIN, ROW3_PIN, ROW4_PIN};
// symbols[Rows][Columns]
#if defined(KBD_3BY4)
#define COLS 3
const uint32_t col_ports[COLS] = {COL1_PORT, COL2_PORT, COL3_PORT};
const uint16_t col_pins[COLS] = {COL1_PIN, COL2_PIN, COL3_PIN};
char kbd[ROWS][COLS] = {
	{'1', '2', '3'},
	{'4', '5', '6'},
	{'7', '8', '9'},
	{'*', '0', '#'}
};
#elif defined(KBD_4BY4)
#define COLS 4
const uint32_t col_ports[COLS] = {COL1_PORT, COL2_PORT, COL3_PORT, COL4_PORT};
const uint16_t col_pins[COLS] = {COL1_PIN, COL2_PIN, COL3_PIN, COL4_PIN};
char kbd[ROWS][COLS] = {
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'*', '0', '#', 'D'}
};
#endif
uint8_t oldstate[ROWS][COLS];

/**
 * init keyboard pins: all columns are opendrain outputs
 * all rows are pullup inputs
 */
void matrixkbd_init(){
	int i, j;
	rcc_peripheral_enable_clock(&RCC_APB2ENR, KBD_RCC_PORT_CLOCK);
	for(i = 0; i < COLS; ++i){
		gpio_set(col_ports[i], col_pins[i]);
		gpio_set_mode(col_ports[i], GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN,
			col_pins[i]);
	}
	for(i = 0; i < ROWS; ++i){
		gpio_set(row_ports[i], row_pins[i]);
		gpio_set_mode(row_ports[i], GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
			row_pins[i]);
	}
	for(j = 0; j < ROWS; ++j)
		for(i = 0; i < COLS; ++i)
			oldstate[j][i] = 1;
}


/**
 * Check keyboard
 * if some key pressed, set prsd to its value (symbol)
 * if released - set rlsd to 1
 * works onse per 60ms
 * DON't work with multiple key pressing!
 * In case of simultaneous pressing it will return at first call code of
 *     first key met, at second - second and so on
 */
void get_matrixkbd(char *prsd, char *rlsd){
	*prsd = 0;
	*rlsd = 0;
	static uint32_t oldTimer = 0;
	if(Timer > oldTimer && Timer - oldTimer < KBD_TIMEOUT) return;
	int r, c;
	for(c = 0; c < COLS; ++c){
		gpio_clear(col_ports[c], col_pins[c]);
		oldTimer = Timer;
		while(oldTimer == Timer); // wait at least one second
		for(r = 0; r < ROWS; ++r){
			uint8_t st = (gpio_get(row_ports[r], row_pins[r])) ? 1 : 0;
			if(oldstate[r][c] != st){ // button state changed
				oldstate[r][c] = st;
				gpio_set(col_ports[c], col_pins[c]);
				if(st == 1){ // released
					*rlsd = 1;
				}else{
					*prsd = kbd[r][c];
				}
				return;
			}
		}
		gpio_set(col_ports[c], col_pins[c]);
	}
}
