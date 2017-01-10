/*
 * matrixkbd.h
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

#include <stdint.h>
#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

void matrixkbd_init();
void get_matrixkbd(char *prsd, char *rlsd);

// timeout for keyboard checkout
#define KBD_TIMEOUT 50

// kbd ports for clock_enable
#define KBD_RCC_PORT_CLOCK    (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN)
// Rows pins & ports: PB15, 13, 11 & 10
#define ROW1_PORT   (GPIOB)
#define ROW1_PIN    (GPIO15)
#define ROW2_PORT   (GPIOB)
#define ROW2_PIN    (GPIO13)
#define ROW3_PORT   (GPIOB)
#define ROW3_PIN    (GPIO11)
#define ROW4_PORT   (GPIOB)
#define ROW4_PIN    (GPIO10)
// Columns pins & ports: PB1, PA7 & PA5
#define COL1_PORT   (GPIOB)
#define COL1_PIN    (GPIO1)
#define COL2_PORT   (GPIOA)
#define COL2_PIN    (GPIO7)
#define COL3_PORT   (GPIOA)
#define COL3_PIN    (GPIO5)
//#define COL4_PORT   ()
//#define COL4_PIN    ()
