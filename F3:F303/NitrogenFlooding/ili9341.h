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
// Enter Sleep Mode
#define ILI9341_SLPIN 0x10
// Sleep Out
#define ILI9341_SLPOUT 0x11
// Partial Mode ON
#define ILI9341_PTLON 0x12
// Normal Display Mode ON
#define ILI9341_NORON 0x13
// Read Display Power Mode
#define ILI9341_RDMODE 0x0A
// Read Display MADCTL
#define ILI9341_RDMADCTL 0x0B
// Read Display Pixel Format
#define ILI9341_RDPIXFMT 0x0C
// Read Display Image Format
#define ILI9341_RDIMGFMT 0x0D
// Read Display Self-Diagnostic Result
#define ILI9341_RDSELFDIAG 0x0F
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
// Memory Read
#define ILI9341_RAMRD 0x2E
// Partial Area
#define ILI9341_PTLAR 0x30
// Vertical Scrolling Definition
#define ILI9341_VSCRDEF 0x33
// Memory Access Control
#define ILI9341_MADCTL 0x36
// Vertical Scrolling Start Address
#define ILI9341_VSCRSADD 0x37
// COLMOD: Pixel Format Set
#define ILI9341_PIXFMT 0x3A



int ili9341_readreg(uint8_t reg, uint8_t *data, uint32_t N);
int ili9341_writereg(uint8_t reg, const uint8_t *data, uint32_t N);
