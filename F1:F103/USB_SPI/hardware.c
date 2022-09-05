/*
 * This file is part of the pl2303 project.
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

#include "hardware.h"

static inline void gpio_setup(){
    // Set APB2 clock to 72/4=18MHz
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_PPRE2) | RCC_CFGR_PPRE2_DIV4;
    // Enable clocks to the GPIO subsystems, turn on AFIO clocking to disable SWD/JTAG
    RCC->APB2ENR = RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_JTAGDISABLE; // for PA15 - USB pullup
    // Set led as opendrain output
    GPIOC->CRH = CRH(13, CNF_ODOUTPUT | MODE_SLOW);
    // setup SPI GPIO - alternate function PP (PA5 - SCK, PA6 - MISO, PA7 - MOSI)
    GPIOA->CRL = CRL(5, CNF_AFPP|MODE_FAST) | CRL(6, CNF_FLINPUT) | CRL(7, CNF_AFPP|MODE_FAST);
    // USB pullup (PA15) - pushpull output
    USBPU_OFF();
    GPIOA->CRH = CRH(15, CNF_PPOUTPUT | MODE_SLOW);
}

void hw_setup(){
    gpio_setup();
}

