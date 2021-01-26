/*
 * This file is part of the pwmtest project.
 * Copyright 2020 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
    // Enable clocks to the GPIO subsystems (PB for ADC), turn on AFIO clocking to disable SWD/JTAG
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
    // turn off SWJ/JTAG
//    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_DISABLE;
    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_JTAGDISABLE; // for PA15
    // Set led as opendrain output
    GPIOC->CRH |= CRH(13, CNF_ODOUTPUT|MODE_SLOW);
    // USB pullup (PA15) - pushpull output
    GPIOA->CRH = CRH(15, CNF_PPOUTPUT|MODE_SLOW) | CRH(8, CNF_AFPP | MODE_FAST);
}

void hw_setup(){
    gpio_setup();
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; // enable TIM1 clocking
    TIM1->ARR = 8; // 9 ticks till UEV
#ifdef HIGHSPEED
    TIM1->PSC = 7;
#else
    TIM1->PSC = 6999;
#endif
    // PWM mode 1 (active->inactive)
    TIM1->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1;
    // main output
    TIM1->BDTR = TIM_BDTR_MOE;
    // main PWM output
    TIM1->CCER = TIM_CCER_CC1E;
#ifndef HIGHSPEED
    NVIC_EnableIRQ(TIM1_CC_IRQn);
#endif
    RCC->AHBENR |= RCC_AHBENR_DMA1EN; // DMA1 clocking
    // memsize 8bit, periphsize 16bit, memincr, circular, mem2periph, half & full transfer interrupt
    DMA1_Channel5->CCR = DMA_CCR_PSIZE_0 | DMA_CCR_CIRC | DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE | DMA_CCR_HTIE;
    DMA1_Channel5->CPAR = (uint32_t)&TIM1->CCR1;
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);
}

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
