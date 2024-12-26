/*
 * This file is part of the hallinear project.
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

#ifndef EBUG
TRUE_INLINE void iwdg_setup(){
    uint32_t tmout = 16000000;
    RCC->CSR |= RCC_CSR_LSION;
    while((RCC->CSR & RCC_CSR_LSIRDY) != RCC_CSR_LSIRDY){if(--tmout == 0) break;} /* (2) */
    IWDG->KR = IWDG_START;
    IWDG->KR = IWDG_WRITE_ACCESS;
    IWDG->PR = IWDG_PR_PR_1;
    IWDG->RLR = 1250;
    tmout = 16000000;
    while(IWDG->SR){if(--tmout == 0) break;}
    IWDG->KR = IWDG_REFRESH;
}
#endif

TRUE_INLINE void gpio_setup(){
    // Set APB2 clock to 72/4=18MHz
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_PPRE2) | RCC_CFGR_PPRE2_DIV4;
    // Enable clocks to the GPIO subsystems
    RCC->APB2ENR = RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_JTAGDISABLE; // for PA15 - USB pullup
    // PA3 - jumper (pullup in), PA7 - ADC in
    GPIOA->CRL = 1<<3;
    GPIOA->CRL = CRL(3, CNF_PUDINPUT | MODE_INPUT) | CRL(7, CNF_ANALOG|MODE_INPUT);
    USBPU_OFF();
    GPIOA->CRH = CRH(15, CNF_PPOUTPUT | MODE_SLOW);
}

void hw_setup(){
    gpio_setup();
#ifndef EBUG
    iwdg_setup();
#endif
}

