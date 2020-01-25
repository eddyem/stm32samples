/*
 *                                                                                                  geany_encoding=koi8-r
 * hardware.c - hardware-dependent macros & functions
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 *
 */

#include "adc.h"
#include "hardware.h"
#include "time.h"
#include "usart.h"

static inline void gpio_setup(){
    // Enable clocks to the GPIO subsystems (PB for ADC), turn on AFIO clocking to disable SWD/JTAG
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
    // turn off SWJ/JTAG
    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_DISABLE;
    // pullups: PA1 - PPS, PA13/PA14 - buttons
    GPIOA->ODR = (1<<12)|(1<<13)|(1<<14)|(1<<15);
    // Set leds (PB8) as opendrain output
    GPIOB->CRH = CRH(8, CNF_ODOUTPUT|MODE_SLOW) | CRH(9, CNF_ODOUTPUT|MODE_SLOW);
    // PPS pin (PA1) - input with weak pullup
    GPIOA->CRL = CRL(1, CNF_PUDINPUT|MODE_INPUT);
    // Set buttons (PA13/14) as inputs with weak pullups, USB pullup (PA15) - opendrain output
    GPIOA->CRH = CRH(13, CNF_PUDINPUT|MODE_INPUT) | CRH(14, CNF_PUDINPUT|MODE_INPUT) |
                 CRH(15, CNF_ODOUTPUT|MODE_SLOW);
    // EXTI: all three EXTI are on PA -> AFIO_EXTICRx = 0
    // interrupt on pulse front: buttons - 1->0, PPS - 0->1
    EXTI->IMR = EXTI_IMR_MR1 | EXTI_IMR_MR13 | EXTI_IMR_MR14; // unmask
    EXTI->RTSR = EXTI_RTSR_TR1; // rising trigger
    EXTI->FTSR = EXTI_FTSR_TR13 | EXTI_FTSR_TR14; // falling trigger
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    NVIC_EnableIRQ(EXTI1_IRQn);
}

static inline void adc_setup(){
    GPIOB->CRL |= CRL(0, CNF_ANALOG|MODE_INPUT);
    uint32_t ctr = 0;
    // Enable clocking
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->CFGR &= ~(RCC_CFGR_ADCPRE);
    RCC->CFGR |= RCC_CFGR_ADCPRE_DIV8; // ADC clock = RCC / 8
    // sampling time - 239.5 cycles for channels 8, 16 and 17
    ADC1->SMPR2 = ADC_SMPR2_SMP8;
    ADC1->SMPR1 = ADC_SMPR1_SMP16 | ADC_SMPR1_SMP17;
    // we have three conversions in group -> ADC1->SQR1[L] = 2, order: 8->16->17
    ADC1->SQR3 = 8 | (16<<5) | (17<<10);
    ADC1->SQR1 = ADC_SQR1_L_1;
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
    //adc_setup();
}

void exti1_isr(){ // PPS - PA1
    systick_correction();
    DBG("exti1");
    EXTI->PR = EXTI_PR_PR1;
}

void exti15_10_isr(){ // PA13 - button0, PA14 - button1
    if(EXTI->PR & EXTI_PR_PR13){
        /*
        if(trigger_ms[0] == DIDNT_TRIGGERED){ // prevent bounce
            trigger_ms[0] = Timer;
            memcpy(&trigger_time[0], &current_time, sizeof(curtime));
        }
        */
        DBG("exti13");
        EXTI->PR = EXTI_PR_PR13;
    }
    if(EXTI->PR & EXTI_PR_PR14){
        /*
        if(trigger_ms[3] == DIDNT_TRIGGERED){ // prevent bounce
            trigger_ms[3] = Timer;
            memcpy(&trigger_time[3], &current_time, sizeof(curtime));
        }
        */
        DBG("exti14");
        EXTI->PR = EXTI_PR_PR14;
    }
}
