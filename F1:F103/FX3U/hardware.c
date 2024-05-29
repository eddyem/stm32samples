/*
 * This file is part of the fx3u project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

/* pinout:

| **Pin #** | **Pin name ** | **function** | **settings**           | **comment **                            |
| --------- | ------------- | ------------ | ---------------------- | --------------------------------------- |
|  15       | PC0/adcin10   | ADC4         | ADC in                 |                                         |
|  16       | PC1/adcin11   | ADC5         | ADC in                 |                                         |
|  23       | PA0           | Y3           | PPOUT                  |                                         |
|  24       | PA1/adcin1    | ADC0         | ADC in                 |                                         |
|  25       | PA2           | Y11          | PPOUT                  |                                         |
|  26       | PA3/adcin3    | ADC1         | ADC in                 |                                         |
|  31       | PA6           | Y10          | PPOUT                  |                                         |
|  32       | PA7           | Y7           | PPOUT                  |                                         |
|  33       | PC4/adcin14   | ADC2         | ADC in                 |                                         |
|  34       | PC5/adcin15   | ADC3         | ADC in                 |                                         |
|  37       | PB2/boot1     | PROG SW      | PUIN                   |                                         |
|  38       | PE7           | X14          | PUIN                   |                                         |
|  39       | PE8           | X15          | PUIN                   |                                         |
|  40       | PE9           | X12          | PUIN                   |                                         |
|  41       | PE10          | X13          | PUIN                   |                                         |
|  42       | PE11          | X10          | PUIN                   |                                         |
|  43       | PE12          | X11          | PUIN                   |                                         |
|  44       | PE13          | X6           | PUIN                   |                                         |
|  45       | PE14          | X7           | PUIN                   |                                         |
|  46       | PE15          | X4           | PUIN                   |                                         |
|  47       | PB10          | X5           | PUIN                   |                                         |
|  48       | PB11          | X2           | PUIN                   |                                         |
|  51       | PB12          | X3           | PUIN                   |                                         |
|  52       | PB13          | X0           | PUIN                   |                                         |
|  53       | PB14          | X1           | PUIN                   |                                         |
|  54       | PB15          | Y6           | PPOUT                  |                                         |
|  59       | PD12          | Y5           | PPOUT                  |                                         |
|  65       | PC8           | Y1           | PPOUT                  |                                         |
|  66       | PC9           | Y0           | PPOUT                  |                                         |
|  67       | PA8           | Y2           | PPOUT                  |                                         |
|  68       | PA9           | RS TX        | AFPP                   |                                         |
|  69       | PA10          | RS RX        | FLIN                   |                                         |
|  76       | PA14/SWCLK    | 485 DE *     | (default)              | (Not now)                               |
|  81       | PD0           | CAN RX       | FLIN                   |                                         |
|  82       | PD1           | CAN TX       | AFPP                   |                                         |
|  89       | PB3/JTDO      | Y4           | PPOUT                  |                                         |
|           |               |              |                        |                                         |
|           |               |              |                        |                                         |
|           |               |              |                        |                                         |
 */

void gpio_setup(void){
    // PD0 & PD1 (CAN) setup in can.c; PA9 & PA10 (USART) in usart.c
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN |
                    RCC_APB2ENR_IOPEEN | RCC_APB2ENR_AFIOEN;
    // Turn off JTAG
    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
    GPIOA->CRL = CRL(0, CNF_PPOUTPUT|MODE_NORMAL) | CRL(1, CNF_ANALOG) | CRL(2, CNF_PPOUTPUT|MODE_NORMAL) |
                 CRL(3, CNF_ANALOG) | CRL(6, CNF_PPOUTPUT|MODE_NORMAL) | CRL(7, CNF_PPOUTPUT|MODE_NORMAL);
    GPIOA->CRH = 0;
    GPIOB->CRL = CRL(2, CNF_PUDINPUT);
    GPIOB->CRH = CRH(10, CNF_PUDINPUT) | CRH(11, CNF_PUDINPUT) | CRH(12, CNF_PUDINPUT) | CRH(13, CNF_PUDINPUT) |
                 CRH(14, CNF_PUDINPUT) | CRH(15, CNF_PPOUTPUT|MODE_NORMAL);
    GPIOC-> CRL = CRL(0, CNF_ANALOG) | CRL(1, CNF_ANALOG) | CRL(4, CNF_ANALOG) | CRL(5, CNF_ANALOG);
    GPIOC->CRH = CRH(8, CNF_PPOUTPUT|MODE_NORMAL) | CRH(9, CNF_PPOUTPUT|MODE_NORMAL);
    GPIOD->CRL = 0;
    GPIOD->CRH = CRH(12, CNF_PPOUTPUT|MODE_NORMAL);
    GPIOE->CRL = CRL(7, CNF_PUDINPUT);
    GPIOE->CRH = CRH(8, CNF_PUDINPUT) | CRH(9, CNF_PUDINPUT) | CRH(10, CNF_PUDINPUT) | CRH(11, CNF_PUDINPUT) |
                 CRH(12, CNF_PUDINPUT) | CRH(13, CNF_PUDINPUT) | CRH(14, CNF_PUDINPUT) | CRH(15, CNF_PUDINPUT);
    // pullups & initial values
    GPIOA->ODR = 0;
    GPIOB->ODR = (1<<2) | (1<<10) | (1<<11) | (1<<12) | (1<<13) | (1<<14);
    GPIOC->ODR = 0;
    GPIOD->ODR = 0;
    GPIOE->ODR = (1<<7) | (1<<8) | (1<<9) | (1<<10) | (1<<11) | (1<<12) | (1<<13) | (1<<14) | (1<<15);
}


void iwdg_setup(){
    uint32_t tmout = 16000000;
    RCC->CSR |= RCC_CSR_LSION;
    while((RCC->CSR & RCC_CSR_LSIRDY) != RCC_CSR_LSIRDY){if(--tmout == 0) break;}
    IWDG->KR = IWDG_START;
    IWDG->KR = IWDG_WRITE_ACCESS;
    IWDG->PR = IWDG_PR_PR_1;
    IWDG->RLR = 1250;
    tmout = 16000000;
    while(IWDG->SR){if(--tmout == 0) break;}
    IWDG->KR = IWDG_REFRESH;
}
