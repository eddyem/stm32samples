/*
 * lcd.c
 *
 * Copyright 2016 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include "main.h"
#include "lcd.h"
#include "hardware_ini.h"
#include "dmagpio.h"
#include "registers.h"

static uint16_t LCD_id = 0;
#define nop() __asm__("nop")
uint8_t readbyte(){
	RD_clear;
	nop();
	uint8_t byte = LCD_rdbyte();
	RD_set;
	nop();
	return byte;
}
void writebyte(uint8_t b){
	LCD_wrbyte(b);
	WR_clear;
	nop();
	WR_set;
	nop();
}
void writereg(uint16_t r){
	LCD_wrbyte(r >> 8);
	RS_clear;
	WR_clear;
	nop();
	WR_set;
	LCD_wrbyte(r & 0xff);
	nop();
	WR_clear;
	nop();
	WR_set;
	RS_set;
}

uint16_t read_reg(uint16_t reg){
	uint32_t dat = 0;
	CS_clear; // active
	writereg(reg);
	LCD_read();
	dat = readbyte() << 8;
	dat |= readbyte();
	CS_set;
	LCD_write(); // restore dir to out
	return dat;
}

void write_reg(uint16_t reg, uint16_t dat){
	CS_clear; // active
	writereg(reg);
	RS_set;  // data
	writebyte(dat >> 8);
	writebyte(dat && 0xff);
	CS_set;
}

/*
typedef union{
	uint8_t  bytes[4];
	uint32_t word;
}word32;

void write16_16(uint16_t reg, uint16_t dat){
	CS_clear; // active
	RS_clear; // register
	LCD_write(); // dir: out
	writebyte(reg >> 8);
	writebyte(reg && 0xff);
	RS_set;  // data
	writebyte(dat >> 8);
	writebyte(dat && 0xff);
	CS_set;
}

void write_reg16_var(uint16_t reg, uint32_t dat, uint8_t len){
	int i;
	CS_clear; // active
	RS_clear; // register
	LCD_write(); // dir: out
	writebyte(reg >> 8);
	writebyte(reg && 0xff);
	RS_set;  // data
	word32 w;
	w.word = dat;
	--len;
	for(i = len; i > -1; --i){
		writebyte(w.bytes[i]);
	}
	CS_set;
}

void write_reg_var(uint8_t reg, uint32_t dat, uint8_t len){
	int i;
	CS_clear; // active
	RS_clear; // register
	LCD_write(); // dir: out
	writebyte(reg);
	RS_set;  // data
	word32 w;
	w.word = dat;
	--len;
	for(i = len; i > -1; --i){
		writebyte(w.bytes[i]);
	}
	CS_set;
}

uint32_t read_reg_var(uint8_t reg, uint8_t len){
	uint32_t dat = 0;
	int i;
	CS_clear; // active
	RS_clear; // register
	LCD_write(); // dir: out
	writebyte(reg);
	LCD_read();
	RS_set;  // data
	for(i = 0; i < len; ++i){
		dat <<= 8;
		dat = dat | readbyte();
	}
	CS_set;
	return dat;
}
uint32_t read16_reg_var(uint16_t reg, uint8_t len){
	uint32_t dat = 0;
	int i;
	CS_clear; // active
	RS_clear; // register
	LCD_write(); // dir: out
	writebyte(reg >> 8);
	writebyte(reg & 0xff);
	LCD_read();
	RS_set;  // data
	for(i = 0; i < len; ++i){
		dat <<= 8;
		dat = dat | readbyte();
	}
	CS_set;
	return dat;
}
*/

uint16_t LCD_status_read(){
	uint16_t dat;
	LCD_read();
	CS_clear;
	RS_clear;
	dat = readbyte() << 8;
	dat |= readbyte();
	RS_set;
	CS_set;
	LCD_write();
	return dat;
}

void LCD_reset(){
	CS_set; RS_set; WR_set; RD_set;
	LCD_write();
	LCD_wrbyte(0xff);
	RST_clear;
	Delay(300);
	RST_set;
	Delay(100);
	write_reg(0,1);
	Delay(100);
/*	CS_clear; RS_clear;
	int i;
	for(i = 0; i < 4; ++i) writebyte(0);
	RS_set; CS_set;
*/
}

/**
 * read LCD identification
 */
uint16_t LCD_read_id(){
	write_reg(0,0);
	LCD_id = read_reg(0);
	write_reg(0,1);
	/*if(read8_32(0x04) == 0x8000){
		write8_24(HX8357D_SETC, 0xFF8357);
		Delay(10);
		if(read8_32(0xD0) == 0x990000){
			LCD_id = 0x8357;
			return 0x8357;
		}
	}
	if(read8_32(0xD3) == 0x9341){
		LCD_id = 0x9341;
		return 0x9341;
	}*/
	return LCD_id;
}

#define TFTLCD_DELAY 0xfff

static const uint16_t ILI932x_regValues[] = {
	ILI932X_START_OSC        , 0x0001, // 0x00
	TFTLCD_DELAY             , 100,
	ILI932X_DISP_CTRL1       , 0x0121, // 0x07
	ILI932X_DRIV_OUT_CTRL    , 0x0100, // 0x01
	ILI932X_DRIV_WAV_CTRL    , 0x0700, // 0x02
	ILI932X_ENTRY_MOD        , 0x0030, // 0x03
	ILI932X_RESIZE_CTRL      , 0x0000, // 0x04
	ILI932X_DISP_CTRL2       , 0x0202, // 0x08
	ILI932X_DISP_CTRL3       , 0x0000, // 0x09
	ILI932X_DISP_CTRL4       , 0x0000, // 0x0A
	ILI932X_RGB_DISP_IF_CTRL1, 0x0001, // 0x0C
	ILI932X_FRM_MARKER_POS   , 0x0000, // 0x0D
	ILI932X_RGB_DISP_IF_CTRL2, 0x0000, // 0x0F
/*
	ILI932X_POW_CTRL1        , 0x0000, // 0x10
	ILI932X_POW_CTRL2        , 0x0007, // 0x11
	ILI932X_POW_CTRL3        , 0x0000, // 0x12
	ILI932X_POW_CTRL4        , 0x0000, // 0x13
	TFTLCD_DELAY             , 200,
	ILI932X_POW_CTRL1        , 0x1690, // 0x10
	ILI932X_POW_CTRL2        , 0x0227, // 0x11
	TFTLCD_DELAY             , 50,
	//ILI932X_POW_CTRL3        , 0x001A, // 0x12
	ILI932X_POW_CTRL3        , 0x001D, // 0x12
	TFTLCD_DELAY             , 50,
	//ILI932X_POW_CTRL4        , 0x1800, // 0x13
	ILI932X_POW_CTRL4        , 0x0800, // 0x13
	//ILI932X_POW_CTRL7        , 0x002A, // 0x29
	ILI932X_POW_CTRL7        , 0x0012, // 0x29
	*/
	ILI932X_POW_CTRL1        , 0x16b0, // 0x10
	ILI932X_POW_CTRL2        , 0x0007, // 0x11
	ILI932X_POW_CTRL3        , 0x0138, // 0x12
	ILI932X_POW_CTRL4        , 0x0b00, // 0x13
	ILI932X_POW_CTRL7        , 0x0000, // 0x29
	TFTLCD_DELAY             , 200,
	ILI932X_FRM_RATE_COL_CTRL, 0x0000, // 0x2B  (100Hz)
	TFTLCD_DELAY             , 50,
	ILI932X_GAMMA_CTRL1      , 0x0000, // 0x30
	ILI932X_GAMMA_CTRL2      , 0x0000, // 0x31
	ILI932X_GAMMA_CTRL3      , 0x0000, // 0x32
	ILI932X_GAMMA_CTRL4      , 0x0206, // 0x35
	ILI932X_GAMMA_CTRL5      , 0x0808, // 0x36
	ILI932X_GAMMA_CTRL6      , 0x0007, // 0x37
	ILI932X_GAMMA_CTRL7      , 0x0201, // 0x38
	ILI932X_GAMMA_CTRL8      , 0x0000, // 0x39
	ILI932X_GAMMA_CTRL9      , 0x0000, // 0x3C
	ILI932X_GAMMA_CTRL10     , 0x0000, // 0x3D
	ILI932X_GRAM_HOR_AD      , 0x0000, // 0x20
	ILI932X_GRAM_VER_AD      , 0x0000, // 0x21
	ILI932X_HOR_START_AD     , 0x0000, // 0x50
	ILI932X_HOR_END_AD       , TFTWIDTH-1, // 0x51
	ILI932X_VER_START_AD     , 0x0000, // 0x52
	ILI932X_VER_END_AD       , TFTHEIGHT-1, // 0x53
	//ILI932X_GATE_SCAN_CTRL1  , 0xA700, // 0x60
	ILI932X_GATE_SCAN_CTRL1  , 0x2700, // 0x60
	//ILI932X_GATE_SCAN_CTRL2  , 0x0003, // 0x61
	ILI932X_GATE_SCAN_CTRL2  , 0x0001, // 0x61
	ILI932X_GATE_SCAN_CTRL3  , 0x0000, // 0x6A
	ILI932X_PANEL_IF_CTRL1   , 0x0010, // 0x90
	ILI932X_PANEL_IF_CTRL2   , 0x0000, // 0x92
	ILI932X_PANEL_IF_CTRL3   , 0x0003, // 0x93
	ILI932X_PANEL_IF_CTRL4   , 0x0110, // 0x95
	ILI932X_PANEL_IF_CTRL5   , 0x0000, // 0x97
	ILI932X_PANEL_IF_CTRL6   , 0x0000, // 0x98
	TFTLCD_DELAY             , 50,
	//ILI932X_DISP_CTRL1       , 0x013b, // 0x07 - 8bit color
	ILI932X_DISP_CTRL1       , 0x0133, // 0x07
};
/*
static const uint16_t ILI932x_regValues[] = {
	ILI932X_START_OSC        , 0x0001, // 0x00
	TFTLCD_DELAY             , 100,
	ILI932X_DRIV_OUT_CTRL    , 0x0000, // 0x01
	ILI932X_DRIV_WAV_CTRL    , 0x0700, // 0x02
	ILI932X_ENTRY_MOD        , 0x1030, // 0x03
	ILI932X_RESIZE_CTRL      , 0x0000, // 0x04
	ILI932X_DISP_CTRL2       , 0x0202, // 0x08
	ILI932X_DISP_CTRL3       , 0x0000, // 0x09
	ILI932X_DISP_CTRL4       , 0x0000, // 0x0A
	ILI932X_RGB_DISP_IF_CTRL1, 0x0003, // 0x0C
	ILI932X_FRM_MARKER_POS   , 0x0000, // 0x0D
	ILI932X_RGB_DISP_IF_CTRL2, 0x0000, // 0x0F
	ILI932X_DISP_CTRL1       , 0x0021, // 0x07
	TFTLCD_DELAY             , 10,
	ILI932X_POW_CTRL1        , 0x0000, // 0x10
	ILI932X_POW_CTRL2        , 0x0007, // 0x11
	ILI932X_POW_CTRL3        , 0x0000, // 0x12
	ILI932X_POW_CTRL4        , 0x0000, // 0x13
	TFTLCD_DELAY             , 100,
	ILI932X_POW_CTRL1        , 0x1690, // 0x10
	ILI932X_POW_CTRL2        , 0x0007, // 0x11
	TFTLCD_DELAY             , 100,
	ILI932X_POW_CTRL3        , 0x0118, // 0x12
	TFTLCD_DELAY             , 100,
	ILI932X_POW_CTRL4        , 0x0b00, // 0x13
	TFTLCD_DELAY             , 100,
	ILI932X_POW_CTRL7        , 0x0012, // 0x29
	ILI932X_FRM_RATE_COL_CTRL, 0x000B, // 0x2B
	ILI932X_GRAM_HOR_AD      , 0x0000, // 0x20
	ILI932X_GRAM_VER_AD      , 0x0000, // 0x21
	ILI932X_HOR_START_AD     , 0x0000, // 0x50
	ILI932X_HOR_END_AD       , TFTWIDTH-1, // 0x51
	ILI932X_VER_START_AD     , 0x0000, // 0x52
	ILI932X_VER_END_AD       , TFTHEIGHT-1, // 0x53
	ILI932X_GATE_SCAN_CTRL1  , 0x2700, // 0x60
	ILI932X_GATE_SCAN_CTRL2  , 0x0001, // 0x61
	ILI932X_GATE_SCAN_CTRL3  , 0x0000, // 0x6A
	ILI932X_PANEL_IF_CTRL1   , 0x0010, // 0x90
	ILI932X_PANEL_IF_CTRL2   , 0x0000, // 0x92
	ILI932X_PANEL_IF_CTRL3   , 0x0001, // 0x93
	ILI932X_PANEL_IF_CTRL4   , 0x0110, // 0x95
	ILI932X_PANEL_IF_CTRL5   , 0x0000, // 0x97
	ILI932X_PANEL_IF_CTRL6   , 0x0000, // 0x98
	ILI932X_DISP_CTRL1       , 0x0133, // 0x07
};*/
/*
static const uint16_t ILI932x_regValues[] = {
	ILI932X_START_OSC        , 0x0001, // 0x00
	TFTLCD_DELAY             , 100,
	ILI932X_DRIV_OUT_CTRL    , 0x0000, // 0x01
	ILI932X_DRIV_WAV_CTRL    , 0x0700, // 0x02
	ILI932X_ENTRY_MOD        , 0x1030, // 0x03
	ILI932X_RESIZE_CTRL      , 0x0000, // 0x04
	ILI932X_DISP_CTRL2       , 0x0202, // 0x08
	ILI932X_DISP_CTRL3       , 0x0000, // 0x09
	ILI932X_DISP_CTRL4       , 0x0000, // 0x0A
	ILI932X_RGB_DISP_IF_CTRL1, 0x0003, // 0x0C
	ILI932X_FRM_MARKER_POS   , 0x0000, // 0x0D
	ILI932X_RGB_DISP_IF_CTRL2, 0x0000, // 0x0F
	ILI932X_DISP_CTRL1       , 0x0011, // 0x07
	TFTLCD_DELAY             , 10,
	ILI932X_POW_CTRL1        , 0x0000, // 0x10
	ILI932X_POW_CTRL2        , 0x0007, // 0x11
	ILI932X_POW_CTRL3        , 0x0000, // 0x12
	ILI932X_POW_CTRL4        , 0x0000, // 0x13
	TFTLCD_DELAY             , 100,
	ILI932X_POW_CTRL1        , 0x1690, // 0x10
	ILI932X_POW_CTRL2        , 0x0007, // 0x11
	TFTLCD_DELAY             , 100,
	ILI932X_POW_CTRL3        , 0x0118, // 0x12
	TFTLCD_DELAY             , 100,
	ILI932X_POW_CTRL4        , 0x0b00, // 0x13
	TFTLCD_DELAY             , 100,
	ILI932X_POW_CTRL7        , 0x0012, // 0x29
	ILI932X_FRM_RATE_COL_CTRL, 0x000B, // 0x2B
	ILI932X_GRAM_HOR_AD      , 0x0000, // 0x20
	ILI932X_GRAM_VER_AD      , 0x0000, // 0x21
	ILI932X_HOR_START_AD     , 0x0000, // 0x50
	ILI932X_HOR_END_AD       , TFTWIDTH-1, // 0x51
	ILI932X_VER_START_AD     , 0x0000, // 0x52
	ILI932X_VER_END_AD       , TFTHEIGHT-1, // 0x53
	ILI932X_GATE_SCAN_CTRL1  , 0x2700, // 0x60
	ILI932X_GATE_SCAN_CTRL2  , 0x0001, // 0x61
	ILI932X_GATE_SCAN_CTRL3  , 0x0000, // 0x6A
	ILI932X_PANEL_IF_CTRL1   , 0x0010, // 0x90
	ILI932X_PANEL_IF_CTRL2   , 0x0000, // 0x92
	ILI932X_PANEL_IF_CTRL3   , 0x0001, // 0x93
	ILI932X_PANEL_IF_CTRL4   , 0x0110, // 0x95
	ILI932X_PANEL_IF_CTRL5   , 0x0000, // 0x97
	ILI932X_PANEL_IF_CTRL6   , 0x0000, // 0x98
	ILI932X_DISP_CTRL1       , 0x0133, // 0x07
};
*/
/**
 * try to init display
 * return 0 if failed
 */
uint16_t LCD_init(){
	//if(LCD_id == 0)
	LCD_read_id();
	if(LCD_id > 0x931f && LCD_id < 0x932a){ // 932x
	//	LCD_id = 0x9320; // make it simple!
		int i, s = sizeof(ILI932x_regValues)/sizeof(uint16_t);
		for(i = 0; i < s;){
			uint16_t a = ILI932x_regValues[i++];
			uint16_t b = ILI932x_regValues[i++];
			if(a == TFTLCD_DELAY) Delay(b);
			else write_reg(a, b);
		}
		uint16_t addr;
		for(addr = 0x80; addr < 0x86; ++addr)
			write_reg(addr, 0);
		Delay(300);
		return LCD_id;
	}
	//LCD_reset();
	return LCD_id;
}

/**
 * put point to current position incrementing coordinates
 */
void putpoint(uint16_t colr){
	int i;
	if(LCD_id == 0) return;
	LCD_write(); // dir: out
	CS_clear; // active
	writereg(ILI932X_RW_GRAM);
	for(i = 0; i < 500; ++i){
		writebyte(colr >> 8);
		writebyte(colr && 0xff);
	}
	CS_set;
	//if(LCD_id == 0x9320){
	//}
}

void setpix(uint16_t x, uint16_t y, uint16_t colr){
	write_reg(ILI932X_GRAM_HOR_AD, x);
	write_reg(ILI932X_GRAM_VER_AD, y);
	write_reg(ILI932X_RW_GRAM, colr);
}
