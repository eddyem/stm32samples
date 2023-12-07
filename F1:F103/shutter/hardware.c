/*
 * This file is part of the shutter project.
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

#include "adc.h"
#include "flash.h"
#include "hardware.h"

static inline void iwdg_setup(){
    uint32_t tmout = 16000000;
    /* Enable the peripheral clock RTC */
    /* (1) Enable the LSI (40kHz) */
    /* (2) Wait while it is not ready */
    RCC->CSR |= RCC_CSR_LSION; /* (1) */
    while((RCC->CSR & RCC_CSR_LSIRDY) != RCC_CSR_LSIRDY){if(--tmout == 0) break;} /* (2) */
    /* Configure IWDG */
    /* (1) Activate IWDG (not needed if done in option bytes) */
    /* (2) Enable write access to IWDG registers */
    /* (3) Set prescaler by 64 (1.6ms for each tick) */
    /* (4) Set reload value to have a rollover each 2s */
    /* (5) Check if flags are reset */
    /* (6) Refresh counter */
    IWDG->KR = IWDG_START; /* (1) */
    IWDG->KR = IWDG_WRITE_ACCESS; /* (2) */
    IWDG->PR = IWDG_PR_PR_1; /* (3) */
    IWDG->RLR = 1250; /* (4) */
    tmout = 16000000;
    while(IWDG->SR){if(--tmout == 0) break;} /* (5) */
    IWDG->KR = IWDG_REFRESH; /* (6) */
}

/*
Pinout:
PB0 (pullup in) - hall (or reed switch) sensor input (active low) - opened shutter detector
PB11 (pullup in) - CCD or button input: open at low signal, close at high

PA3 (ADC in) - shutter voltage (approx 1/12 U)
PA5 (PP out) - TLE5205 IN2
PA6 (PP out) - TLE5205 IN1
PA7 (pullup in) - TLE5205 FB
PA10 (PP out) - USB pullup (active low)
PA11,12 - USB
PA13,14 - SWD
*/

static inline void gpio_setup(){
    // Enable clocks to the GPIO subsystems, turn on AFIO clocking to disable SWD/JTAG
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    __DSB();
    // turn off SWJ/JTAG
//    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_DISABLE;
    // AFIO->MAPR = AFIO_MAPR_SWJ_CFG_JTAGDISABLE; // for PA15
    // TLE5205 in OFF state
    // ADC (PA3), OD out (PA5,6), PU in (PA7)
    GPIOA->ODR = 1 << 7;
    SHTROFF();
    GPIOA->CRL = CRL(3, CNF_ANALOG | MODE_INPUT) | CRL(5, CNF_PPOUTPUT|MODE_SLOW) | CRL(6, CNF_PPOUTPUT|MODE_SLOW) |
                 CRL(7, CNF_PUDINPUT | MODE_INPUT);
    // USB pullup (PA10) - pushpull output
    GPIOA->CRH = CRH(8, CNF_PPOUTPUT | MODE_SLOW) | CRH(10, CNF_PPOUTPUT | MODE_SLOW);
    // hall/ccd: pulled up or down depending on settings
    // if ccdactive==0 - shutter is closed when no output signals
    GPIOB->ODR = ((the_conf.hallactive) ? 0: 1) | ((the_conf.ccdactive) ? 0 : 1<<11);
    GPIOB->CRL = CRL(0, CNF_PUDINPUT | MODE_INPUT);
    GPIOB->CRH = CRH(11, CNF_PUDINPUT | MODE_INPUT);
}

void hw_setup(){
    iwdg_setup();
    gpio_setup();
    adc_setup();
}

uint32_t getShutterVoltage(){
    uint32_t val = getADCvoltage(CHSHTR);
    val *= the_conf.shtrVmul;
    return val / the_conf.shtrVdiv;
}
