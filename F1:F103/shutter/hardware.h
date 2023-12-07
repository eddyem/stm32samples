/*
 * This file is part of the shutter project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32f1.h>

#include "flash.h"

// PA7 - SHTR_FB
#define FBPORT      GPIOA
#define FBPIN       (1<<7)
// PA6, PA5 - SHTR_A, SHTR_B
#define SHTRPORT    GPIOA
#define SHTRAPOS    (6)
#define SHTRAPIN    (1<<SHTRAPOS)
#define SHTRBPOS    (5)
#define SHTRBPIN    (1<<SHTRBPOS)

#define CHKFB()     (FBPORT->IDR & FBPIN ? 0 : 1)
// open: 00
#define SHTROPEN()  do{SHTRPORT->BSRR = (SHTRAPIN | SHTRBPIN) << 16;}while(0)
// close: 01
#define SHTRCLOSE() do{SHTRPORT->BSRR = (SHTRAPIN << 16) | SHTRBPIN;}while(0)
// off: 10
#define SHTROFF()  do{SHTRPORT->BSRR = (SHTRBPIN << 16) | SHTRAPIN;}while(0)
// hiZ: 11
#define SHTRHIZ()   do{SHTRPORT->BSRR = SHTRAPIN | SHTRBPIN;}while(0)
#define SHTRSTATE() (((SHTRPORT->IDR & SHTRAPIN) >> (SHTRAPOS-1)) | ((SHTRPORT->IDR & SHTRBPIN) >> SHTRBPOS))
// states of TLE5205
typedef enum{
    REG_OPEN,
    REG_CLOSE,
    REG_OFF,
    REG_HIZ
} regul_state;

// PB0 - hall (==1 when opened), PB11 - CCD (active low)
#define BTNPORT     GPIOB
#define HALLPIN     (1<<0)
#define CCDPIN      (1<<11)
#define CHKHALL()   ((HALLPIN == (BTNPORT->IDR & HALLPIN)) == the_conf.hallactive)
#define CHKCCD()    ((CCDPIN == (BTNPORT->IDR & CCDPIN)) == the_conf.ccdactive)

extern volatile uint32_t Tms;

void hw_setup();
uint32_t getShutterVoltage();
