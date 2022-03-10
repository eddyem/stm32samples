/*
 * lcd.h
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

#pragma once
#ifndef __LCD_H__
#define __LCD_H__

#define TFTWIDTH   240
#define TFTHEIGHT  320

#define    DWT_CYCCNT    *(volatile uint32_t *)0xE0001004
#define    DWT_CONTROL   *(volatile uint32_t *)0xE0001000
#define    SCB_DEMCR     *(volatile uint32_t *)0xE000EDFC
// pause for 150ns

#define pause()  do{SCB_DEMCR  |= 0x01000000; DWT_CYCCNT = 0; DWT_CONTROL|= 1; \
						while(DWT_CYCCNT < 3);}while(0)

//#define pause() do{}while(0)

uint16_t read_reg(uint16_t reg);
void write_reg(uint16_t reg, uint16_t dat);

/*
uint32_t read_reg_var(uint8_t reg, uint8_t len);
uint32_t read16_reg_var(uint16_t reg, uint8_t len);

#define write8_32(r,d) do{write_reg_var(r,d,4);}while(0)
#define write8_24(r,d) do{write_reg_var(r,d,3);}while(0)
#define write8_16(r,d) do{write_reg_var(r,d,2);}while(0)
#define write16_32(r,d) do{write_reg16_var(r,d,4);}while(0)
#define write16_24(r,d) do{write_reg16_var(r,d,3);}while(0)
//#define write16_16(r,d) do{write_reg16_var(r,d,2);}while(0)
void write16_16(uint16_t reg, uint16_t dat);
#define read8_32(x)   (read_reg_var(x, 4))
#define read8_16(x)    ((uint16_t)(read_reg_var(x, 2)))
#define read16_32(x)   (read16_reg_var(x, 4))
#define read16_16(x)    ((uint16_t)(read16_reg_var(x, 2)))
*/

uint16_t LCD_status_read();
uint16_t LCD_read_id();
void LCD_reset();
uint16_t LCD_init();
void putpoint(uint16_t colr);
void setpix(uint16_t x, uint16_t y, uint16_t colr);



#endif // __LCD_H__
