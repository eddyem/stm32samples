/*
 * pcd8544.h
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

#pragma once
#ifndef __PCD8544_H__
#define __PCD8544_H__

#include "font.h"

// sizes in graphics (84x48) & char(letter 6x8 -> 14x6) mode
#define XSIZE    (84)
#define XCHARSZ  (XSIZE/LTR_WIDTH)
#define YSIZE    (48)
#define YCHARSZ  (YSIZE/8)

// type of screen size
typedef U8 scrnsz_t;
// type of screen buffer index (scrnsz_t^2)
typedef U16 bufsz_t;

extern const scrnsz_t LTRS_IN_ROW;
extern const scrnsz_t ROW_MAX;

#define bad_coords(x, y)            (!(x < XSIZE && y < YSIZE))
#define bad_text_coords(col, row)   (!(col < LTRS_IN_ROW && row < ROW_MAX))

/*
// bounding box
typedef struct{
	scrnsz_t xmin,
	scrnsz_t ymin,
	scrnsz_t xmax,
	scrnsz_t ymax
} bbox_t;
*/

void pcd8544_init();
void pcd8544_send_byte(U8 byte, U8 cmd);
void pcd8544_send_data(U8 *data, bufsz_t size, U8 cmd);
void draw_pixel(scrnsz_t x, scrnsz_t y, U8 set);
void pcd8544_cls();
void pcd8544_refresh();
int  pcd8544_put(U8 koi8, scrnsz_t x, scrnsz_t y);
U8 *pcd8544_print(U8 *koi8);
U8 pcd8544_putch(U8 koi8);
void pcd8544_roll_screen();

void pcd8544_setbias(U8 val);
void pcd8544_setvop(U8 val);
void pcd8544_settemp(U8 val);

#define set_pixel(x,y)    do{draw_pixel(x,y,1);}while(0)
#define clear_pixel(x,y)  do{draw_pixel(x,y,0);}while(0)

/********************** PCD8544 protocol **********************/
// Function set: | 0 | 0 | 1 | 0 | 0 | PD | V | H |
#define PCD8544_POWERDOWN           (0x24)
#define PCD8544_VERTADDRESSING      (0x22)
#define PCD8544_EXTENDEDINSTRUCTION (0x21)
#define PCD8544_DEFAULTMODE         (0x20)
/*
 *                    functions with H=0 (basic)
 */
// Display control: | 0 | 0 | 0 | 0 | 1 | D | 0 | E |
#define PCD8544_DISPLAYBLANK      (8)
#define PCD8544_DISPLAYFILLED     (9)
#define PCD8544_DISPLAYNORMAL     (0xc)
#define PCD8544_DISPLAYINVERTED   (0xd)
// Set Xaddr: | 1 | X6 | X5 | X4 | X3 | X2 | X1 | X0 |
#define PCD8544_SETXADDR   (0x80)
#define PCD8544_XADDR_MASK (0x7f)
// Set Yaddr: | 0 | 1 | 0 | 0 | 0 | Y2 | Y1 | Y0 |
#define PCD8544_SETYADDR   (0x40)
#define PCD8544_YADDR_MASK (7)
/*
 *                    functions with H=1 (extended)
 */
// Temperature control: | 0 | 0 | 0 | 0 | 0 | 1 | TC1 | TC0 |
#define PCD8544_SETTEMP    (4)
#define PCD8544_TEMP_MASK  (3)
// BIAS: | 0 | 0 | 0 | 1 | 0 | BS2 | BS1 | BS0 |
#define PCD8544_SETBIAS    (0x10)
#define PCD8544_BIAS_MASK  (7)
// VOP:  | 1 | VOP6 | VOP5 | VOP4 | VOP3 | VOP2 | VOP1 | VOP0 |
#define PCD8544_SETVOP     (0x80)
#define PCD8544_VOP_MASK   (0x7f)
// | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

/********************** Shortened macros **********************/
#define CMD(x)    do{pcd8544_send_byte(x, 1);}while(0)
#define DAT(x)    do{pcd8544_send_byte(x, 0);}while(0)
#define CBUF(x,l) do{pcd8544_send_data(x, l, 1);}while(0)
#define DBUF(x,l) do{pcd8544_send_data(x, l, 0);}while(0)
// set bias level (look into datasheet, 4 - recommend)
#define SETBIAS(x)  CMD(PCD8544_SETBIAS|(x&PCD8544_BIAS_MASK))
// set operation voltage (=3.06+0.06*x)
#define SETVOP(x)   CMD(PCD8544_SETVOP |(x&PCD8544_VOP_MASK))
// temperature compensation: 0 - high, 1 - recommended, 2 - IC, 3 - minimal
#define SETTEMP(x)  CMD(PCD8544_SETTEMP|(x&PCD8544_TEMP_MASK))
// x,y addressing
#define SETXADDR(x) CMD(PCD8544_SETXADDR|(x&PCD8544_XADDR_MASK))
#define SETYADDR(x) CMD(PCD8544_SETYADDR|(x&PCD8544_YADDR_MASK))
#endif // __PCD8544_H__
