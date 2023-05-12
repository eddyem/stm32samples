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

#include <stm32f3.h>

// in 4-wire SPI mode there's 16-bit color data: (MSB) 5R-6G-5B (LSB)
#define RGB(r,g,b)  (((r&0x1f)<<11)|((g&0x3f)<<5)|((b&0x1F)))

/***** Registers *****/
// No-op register
#define ILI9341_NOP 0x00
// Software reset register
#define ILI9341_SWRESET 0x01
// Read display identification information
#define ILI9341_RDDID 0x04
// Read Display Status
#define ILI9341_RDDST 0x09
// Read Display Power Mode
#define ILI9341_RDMODE 0x0A
// Read Display MADCTL
#define ILI9341_RDMADCTL 0x0B
// Read Display Pixel Format
#define ILI9341_RDPIXFMT 0x0C
// Read Display Image Format
#define ILI9341_RDIMGFMT 0x0D
// Read Display Signal Mode
#define ILI9341_RDSIGMODE   0x0E
// Read Display Self-Diagnostic Result
#define ILI9341_RDSELFDIAG 0x0F
// Enter Sleep Mode
#define ILI9341_SLPIN 0x10
// Sleep Out
#define ILI9341_SLPOUT 0x11
// Partial Mode ON
#define ILI9341_PTLON 0x12
// Normal Display Mode ON
#define ILI9341_NORON 0x13
// Display Inversion OFF
#define ILI9341_INVOFF 0x20
// Display Inversion ON
#define ILI9341_INVON 0x21
// Gamma Set
#define ILI9341_GAMMASET 0x26
// Display OFF
#define ILI9341_DISPOFF 0x28
// Display ON
#define ILI9341_DISPON 0x29
// Column Address Set
#define ILI9341_CASET 0x2A
// Page Address Set
#define ILI9341_PASET 0x2B
// Memory Write
#define ILI9341_RAMWR 0x2C
// Color Set
#define ILI9341_COLRSET 0x2D
// Memory Read
#define ILI9341_RAMRD 0x2E
// Partial Area
#define ILI9341_PTLAR 0x30
// Vertical Scrolling Definition
#define ILI9341_VSCRDEF 0x33
// Tearing Effect Line OFF
#define ILI9341_TEAROFF 0x34
// Tearing Effect Line ON
#define ILI9341_TEARON 0x35
// Memory Access Control
#define ILI9341_MADCTL 0x36
// Vertical Scrolling Start Address
#define ILI9341_VSCRSADD 0x37
// Idle Mode OFF
#define ILI9341_IDLEOFF 0x38
// Idle Mode ON
#define ILI9341_IDLEON 0x39
// COLMOD: Pixel Format Set
#define ILI9341_COLMOD 0x3A
// Write Memory Continue
#define ILI9341_WRMEMCONT 0x3C
// Read Memory Continue
#define ILI9341_RDMEMCONT 0x3E
// Write Display Brightness
#define ILI9341_WRBRIGHT 0x51
// Write CTRL Display
#define ILI9341_WRCTRLDIS 0x53
// RGB Interface Signal Control
#define ILI9341_RGBCTL 0xB0
// Frame Rate Control (In Normal Mode/Full Colors)
#define ILI9341_FRCTLN 0xB1
// Frame Rate Control (In Idle Mode/8 colors)
#define ILI9341_FRCTLI 0xB2
// Frame Rate control (In Partial Mode/Full Colors)
#define ILI9341_FRCTLP 0xB3
// Display Inversion Control
#define ILI9341_INVCTL 0xB4
// Blanking Porch Control
#define ILI9341_BLPOCTL 0xB5
// Display Function Control
#define ILI9341_DFUNCTL 0xB6
// Entry Mode Set
#define ILI9341_ENTRMODSET 0xB7
// Backlight Control x (x=1..5, 7,8)
#define ILI9341_BACKLCTL(x) (0xB7+x)
// Power Control x (x=1..)
#define ILI9341_POWCTL(x) (0xBF+x)
// VCOM Control 1
#define ILI9341_VCOMCTL1 0xC5
// VCOM Control 2
#define ILI9341_VCOMCTL2 0xC7
// Power control A
#define ILI9341_POWCTLA 0xCB
// Power control B
#define ILI9341_POWCTLB 0xCF
// Positive Gamma Correction
#define ILI9341_POSGAMCOR 0xE0
// Negative Gamma Correction
#define ILI9341_NEGGAMCOR 0xE1
// Driver timing control A1
#define ILI9341_DRVTCTLA1 0xE8
// Driver timing control A2
#define ILI9341_DRVTCTLA2 0xE9
// Driver timing control B
#define ILI9341_DRVTCTLB 0xEA
// Power on sequence control
#define ILI9341_POWONSEQCTL 0xED
// Enable 3G
#define ILI9341_3GEN 0xF2
// Interface Control
#define ILI9341_IFACECTL 0xF6
// Pump ratio control
#define ILI9341_PUMPRATCTL 0xF7

// MADCTL bits:
// row address (==1 - upside down)- 0x80
#define ILI9341_MADCTL_MY  (1<<7)
// column address (==1 - right to left) - 0x40
#define ILI9341_MADCTL_MX  (1<<6)
// row/column exchange - 0x20
#define ILI9341_MADCTL_MV  (1<<5)
// vertical refresh direction - 0x10
#define ILI9341_MADCTL_ML  (1<<4)
// == 1 for RGB - 0x08
#define ILI9341_MADCTL_RGB (1<<3)
// horizontal refresh direction - 0x04
#define ILI9341_MADCTL_MH  (1<<2)

#define SCRNSZMAX 320
#define SCRNSZMIN 240
#define SCRNSZ (SCRNSZMAX*SCRNSZMIN)

// Different orientations
// default orientation: w=320, h=240, zero in upper left corner, connector is from the left
#define DEFMADCTL (ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_RGB)
#define SCRNW SCRNSZMAX
#define SCRNH SCRNSZMIN

#define COLORBUFSZ (2*SCRNW)
extern uint16_t colorbuf[];

int ili9341_init();
int ili9341_on();
int ili9341_off();
int ili9341_readreg(uint8_t reg, uint8_t *data, uint32_t N);
int ili9341_writereg(uint8_t reg, const uint8_t *data, uint32_t N);
int ili9341_writereg16(uint8_t reg, uint16_t data);
int ili9341_writereg32(uint8_t reg, uint16_t data1, uint16_t data2);
int ili9341_writecmd(uint8_t cmd);
int ili9341_writedata(uint8_t *data, uint32_t N);
int ili9341_readdata(uint8_t *data, uint32_t N);
int ili9341_readregdma(uint8_t reg, uint8_t *data, uint32_t N);

int ili9341_fill(uint16_t color);
int ili9341_filln(uint16_t color);
int ili9341_fillp(uint16_t color, int sz);
int ili9341_setcol(uint16_t start, uint16_t stop);
int ili9341_setrow(uint16_t start, uint16_t stop);
