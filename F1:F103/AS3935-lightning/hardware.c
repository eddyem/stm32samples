/*
 * This file is part of the as3935 project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "hardware.h"
#include "spi.h"

/*
 * PA0 - INT0 - PD in
 * PA1 - INT1 - PD in
 * PA2 - CS0  - PP out
 * PA3 - CS1  - PP out
 * PA5 - SPI1 SCK
 * PA6 - SPI1 MISO
 * PA7 - SPI1 MOSI
 * PA9 - USART1 Tx
 * PA10- USART1 Rx
 */

static inline void gpio_setup(){
    // Enable clocks to the GPIO subsystems (PB for ADC), turn on AFIO clocking to disable SWD/JTAG
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_SPI1EN;
    // turn off SWJ/JTAG
    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_JTAGDISABLE; // for PA15
    // Set led as opendrain output
    GPIOC->CRH |= CRH(13, CNF_ODOUTPUT|MODE_SLOW);
    // PA pins
    GPIOA->ODR = 0; // for pull-down
    GPIOA->CRL = CRL(0, CNF_PUDINPUT|MODE_INPUT) | CRL(1, CNF_PUDINPUT|MODE_INPUT) | CRL(2, CNF_PPOUTPUT|MODE_SLOW) |
                 CRL(3, CNF_PPOUTPUT|MODE_SLOW) | CRL(5, CNF_AFPP|MODE_FAST) | CRL(6, CNF_FLINPUT|MODE_INPUT) | CRL(7, CNF_AFPP|MODE_FAST);
    GPIOA->CRH = CRH(9, CNF_AFPP|MODE_FAST) | CRH(10, CNF_FLINPUT|MODE_INPUT) | CRH(15, CNF_PPOUTPUT|MODE_SLOW);
    CS_OFF();
}

void hw_setup(){
    gpio_setup();
    adc_setup();
    spi_setup();
}

#ifndef EBUG
void iwdg_setup(){
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

