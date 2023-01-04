/*
 * This file is part of the F1_testbrd project.
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

#include "adc.h"
#include "hardware.h"
#include "usart.h"

static inline void gpio_setup(){
    // Enable clocks to the GPIO subsystems (PB for ADC), turn on AFIO clocking to disable SWD/JTAG
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_JTAGDISABLE; // for PA15
    // turn off USB pullup
    GPIOA->ODR = (1<<15); // turn off usb pullup & turn on pullups for buttons
    // Set LEDS (PA6-8, PB0/1) as Out & AF (PWM)
    GPIOA->CRL = CRL(6, CNF_AFOD|MODE_NORMAL) | CRL(7, CNF_AFOD|MODE_NORMAL);
    // USB pullup (PA15) - pushpull output
    GPIOA->CRH = CRH(8, CNF_AFOD|MODE_NORMAL) | CRH(15, CNF_PPOUTPUT|MODE_SLOW);
    GPIOB->CRL = CRL(0, CNF_AFOD|MODE_NORMAL) | CRL(1, CNF_AFOD|MODE_NORMAL);
}

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

// TIM3 - PWM @ all 4 channels. TIM1 - PWM @ ch1
static inline void tim1_setup(){
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; // enable TIM1 clocking
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    TIM1->PSC = 8;  // 72/9 = 8MHz
    TIM3->PSC = 8;
    // ARR for 8-bit PWM
    TIM1->ARR = 255; // 100 ticks for 80kHz
    TIM3->ARR = 255;
    TIM1->CCR1 = 127; // 50%
    TIM3->CCR1 = 63; TIM3->CCR2 = 127; TIM3->CCR3 = 191; TIM3->CCR4 = 250;
    // PWM mode 2
    TIM1->CCMR1 = TIM_CCMR1_OC1M;
    TIM3->CCMR1 = TIM_CCMR1_OC1M | TIM_CCMR1_OC2M;
    TIM3->CCMR2 = TIM_CCMR2_OC3M | TIM_CCMR2_OC4M;
    // main output
    TIM1->BDTR = TIM_BDTR_MOE;
    TIM3->BDTR |= TIM_BDTR_MOE;
    // main PWM output
    TIM1->CCER = TIM_CCER_CC1E;
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;
    // turn it on
    TIM1->CR1 = TIM_CR1_CEN;// | TIM_CR1_ARPE;
    TIM3->CR1 |= TIM_CR1_CEN;
    TIM1->EGR |= TIM_EGR_UG; // generate update event to refresh all
    TIM3->EGR |= TIM_EGR_UG;
}

static inline void adc_setup(){
    GPIOA->CRL |= CRL(0, CNF_ANALOG|MODE_INPUT) | CRL(1, CNF_ANALOG|MODE_INPUT) | CRL(5, CNF_ANALOG|MODE_INPUT);
    uint32_t ctr = 0;
    // Enable clocking
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->CFGR &= ~(RCC_CFGR_ADCPRE);
    RCC->CFGR |= RCC_CFGR_ADCPRE_DIV8; // ADC clock = RCC / 8
    // sampling time - 239.5 cycles for channels 0, 1, 5, 16 and 17
    ADC1->SMPR2 = ADC_SMPR2_SMP0 | ADC_SMPR2_SMP1 | ADC_SMPR2_SMP5;
    ADC1->SMPR1 = ADC_SMPR1_SMP16 | ADC_SMPR1_SMP17;
    // sequence order: 0 -> 1 -> 5 -> 16 -> 17
    ADC1->SQR3 = 0 | (1<<5) | (5<<10) | (16<<15) | (17<<20);
    ADC1->SQR1 = (NUMBER_OF_ADC_CHANNELS - 1) << 20; // amount of conversions
    ADC1->CR1 |= ADC_CR1_SCAN; // scan mode
    // DMA configuration
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel1->CPAR = (uint32_t) (&(ADC1->DR));
    DMA1_Channel1->CMAR = (uint32_t)(ADC_array);
    DMA1_Channel1->CNDTR = NUMBER_OF_ADC_CHANNELS * 9;
    DMA1_Channel1->CCR |= DMA_CCR_MINC | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0
                          | DMA_CCR_CIRC | DMA_CCR_PL | DMA_CCR_EN;
    // continuous mode & DMA; enable vref & Tsens; wake up ADC
    ADC1->CR2 |= ADC_CR2_DMA | ADC_CR2_TSVREFE | ADC_CR2_CONT | ADC_CR2_ADON;
    // wait for Tstab - at least 1us
    while(++ctr < 0xff) nop();
    // calibration
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    ctr = 0; while((ADC1->CR2 & ADC_CR2_RSTCAL) && ++ctr < 0xfffff);
    ADC1->CR2 |= ADC_CR2_CAL;
    ctr = 0; while((ADC1->CR2 & ADC_CR2_CAL) && ++ctr < 0xfffff);
    // turn ON ADC
    ADC1->CR2 |= ADC_CR2_ADON;
}

void hw_setup(){
    gpio_setup();
    adc_setup();
    tim1_setup();
    iwdg_setup();
}
