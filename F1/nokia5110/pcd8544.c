/*
 * pcd8544.c - functions for work with display controller
 *
 * Copyright 2015 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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

/*
 * we have two modes: direct write into LCD (with local buffer update) - for text
 * and refresh whole buffer - for graphics
 * all writings are in horizontal addressing mode
 */

#include "pcd8544.h"
// here should be functions spiWrite(U8 *data, bufsz_t size) & spi_write_byte(U8 onebyte)
// max SPI speed is 4MHz
#include "spi.h"
/* here are functions & macros for changing command pins state:
 *   SET_DC()    - set D/~C pin high (data)
 *   CLEAR_DC()  - clear D/~C (command)
 *   CHIP_EN()   - clear ~SCE
 *   CHIP_DIS()  - set ~SCE (disable chip)
 *   CLEAR_RST() - set 1 on RST pin
 *   LCD_RST()   - set 0 on RST pin
 */
#include "hw_init.h"
#include <string.h>

const scrnsz_t LTRS_IN_ROW = XSIZE / LTR_WIDTH;
const scrnsz_t ROW_MAX = YSIZE / 8;
// array for fast pixel set/reset
const U8 const pixels_set[] = {1,2,4,8,0x10,0x20,0x40,0x80};
const U8 const pixels_reset[] = {0xfe,0xfd,0xfb,0xf7,0xef,0xdf,0xbf,0x7f};
/**
 * I use horizontal addressing of PCD8544, so data in buffer
 * stored line by line, each byte is 8 vertical pixels (LSB upper)
 *
 * As refresh speed is fast enough, I don't use partial update for graphics
 * (but use for letters)
 */
#define DISPLAYBUFSIZE   (XSIZE*YCHARSZ)
static U8 displaybuf[DISPLAYBUFSIZE];

// current letter coordinates - for "printf"
static scrnsz_t cur_x = 0, cur_y = 0;
extern U8 bias, vop, temp;

void pcd8544_setbias(U8 val){
	CMD(PCD8544_EXTENDEDINSTRUCTION);
	SETBIAS(val);
	CMD(PCD8544_DEFAULTMODE);
}

void pcd8544_setvop(U8 val){
	CMD(PCD8544_EXTENDEDINSTRUCTION);
	SETVOP(val);
	CMD(PCD8544_DEFAULTMODE);
}

void pcd8544_settemp(U8 val){
	CMD(PCD8544_EXTENDEDINSTRUCTION);
	SETTEMP(val);
	CMD(PCD8544_DEFAULTMODE);
}

/**
 * Init SPI & display
 */
void pcd8544_init(){
	CHIP_DIS();
	SET_DC();
	LCD_RST();
	Delay(10);
	CLEAR_RST(); // set ~RST to 1, performing out of RESET stage
//	CMD(0x21); CMD(0xc8); CMD(0x06); CMD(0x13); CMD(0x20); CMD(0x0c);

	// set LCD to optimal mode by datasheet: bias 1/48 (n=4), Vlcd 6.06 (Vop=50), normal Tcorr (1)
	CMD(PCD8544_EXTENDEDINSTRUCTION);
	SETBIAS(4);
	SETVOP(65);
	SETTEMP(1);
	// return to regular instructions set, horizontal addressing & prepare normal display mode
	CMD(PCD8544_DEFAULTMODE);
	CMD(PCD8544_DISPLAYNORMAL);

	pcd8544_cls();
}

/**
 * Send command (cmd != 0) or data (cmd == 0) byte
 */
void pcd8544_send_byte(U8 byte, U8 cmd){
	CHIP_EN();
	if(cmd)
		CLEAR_DC();
	else
		SET_DC();
	spi_write_byte(byte);
}

/**
 * Send data sequence
 */
void pcd8544_send_data(U8 *data, bufsz_t size, U8 cmd){
	CHIP_EN();
	if(cmd)
		CLEAR_DC();
	else
		SET_DC();
	spiWrite(data, size);
	CHIP_DIS();
}

void draw_pixel(scrnsz_t x, scrnsz_t y, U8 set){
	bufsz_t idx;
	if(bad_coords(x,y)) return;
	idx = x + (y/8) * XSIZE;
	y %= 8;
	if(set)
		displaybuf[idx] |= pixels_set[y];
	else
		displaybuf[idx] &= pixels_reset[y];
}

void pcd8544_cls(){
	memset(displaybuf, 0, DISPLAYBUFSIZE);
	cur_x = cur_y = 0;
	pcd8544_refresh();
}

/**
 * send full data buffer onto display
 */
void pcd8544_refresh(){
	SETXADDR(0);
	SETYADDR(0);
	DBUF(displaybuf, DISPLAYBUFSIZE);
}

/**
 * draw letter at x,y position in char coordinates (XCHARSZ x YCHARSZ)
 * @return 0 if fail
 */
int pcd8544_put(U8 koi8, scrnsz_t x, scrnsz_t y){
	U8 *symbol;
	bufsz_t idx;
	if(x >= XCHARSZ || y >= YCHARSZ) return 0;
	if(koi8 < 32) return 0;
	symbol = (U8*)letter(koi8);
	x *= LTR_WIDTH;
	idx = x + y*XSIZE;
	// put letter into display buffer
	memcpy(&displaybuf[idx], symbol, LTR_WIDTH);
	// and show it on display
	SETXADDR(x);
	SETYADDR(y);
	DBUF(symbol, LTR_WIDTH);
	return 1;
}

/**
 * put char into current position or next line; return char if couldn't be printed
 * (if OK return 0 or '\n' if newline was made)
 */
U8 pcd8544_putch(U8 chr){
	scrnsz_t tabpos;
	U8 ret = 0;
	if(cur_x >= XCHARSZ || cur_y >= YCHARSZ) return chr;
	if(chr < 32){ // special symbols
		switch(chr){
			case '\n': // carriage return - return to beginning of current line
				++cur_y;
			// here shouldn't be a "break" because '\n' == "\r\n"
			case '\r': // newline
				cur_x = 0;
			break;
			case '\b': // return to previous position (backspace)
				if(cur_x) --cur_x;
			break;
			case '\t': // tabulation by 4 symbols
				tabpos = ((cur_x>>2)<<2) + 4;
				if(tabpos < XCHARSZ) cur_x = tabpos;
				else return chr;
			break;
		}
	}else{
		// increment x position here (there could be not writeable letters)
		pcd8544_put(chr, cur_x++, cur_y);
	}
	if(cur_x == XCHARSZ){
		cur_x = 0;
		++cur_y;
		return '\n';
	}
	return ret;
}

/**
 * print zero-terminated string from current (cur_x, cur_y)
 * truncate long strings that don't fit into display
 * @return NULL if all OK or pointer to non-printed string
 */
U8 *pcd8544_print(U8 *koi8){
	while(*koi8){
		U8 chr = *koi8;
		if(pcd8544_putch(chr) == chr) return koi8;
		++koi8;
	}
	return NULL;
}

/**
 * roll screen by 1 line up
 */
void pcd8544_roll_screen(){
	bufsz_t idx = DISPLAYBUFSIZE-XSIZE;
	memmove(displaybuf, displaybuf + XSIZE, idx);
	memset(displaybuf+idx, 0, XSIZE);
	pcd8544_refresh();
	if(cur_y) --cur_y;
}

