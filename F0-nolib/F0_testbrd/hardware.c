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
#include "usart.h"

static inline void gpio_setup(){
    // here we turn on clocking for all periph.
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_DMAEN;
    // Set LEDS (PA0/4) as Oun & AF (PWM)
    GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER4)
                    ) |
                    GPIO_MODER_MODER0_O | GPIO_MODER_MODER4_AF ;
    pin_set(LED0_port, LED0_pin); // clear LEDs
    pin_set(LED1_port, LED1_pin);
    // Buttons - PA14/15
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(GPIO_PUPDR_PUPDR14 | GPIO_PUPDR_PUPDR15)
                    ) |
                    GPIO_PUPDR_PUPDR14_0 | GPIO_PUPDR_PUPDR15_0;
    // alternate functions: PA4 - TIM14_CH1 (AF4)
    GPIOA->AFR[0] = (GPIOA->AFR[0] &~ (GPIO_AFRL_AFRL4)) \
                | (4 << (4 * 4)) ;
}

static inline void adc_setup(){
    GPIOB->MODER = GPIO_MODER_MODER0_AI; // PB0 - ADC channel 8
    uint16_t ctr = 0; // 0xfff0 - more than 1.3ms
    // Enable clocking
    /* (1) Enable the peripheral clock of the ADC */
    /* (2) Start HSI14 RC oscillator */
    /* (3) Wait HSI14 is ready */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; /* (1) */
    RCC->CR2 |= RCC_CR2_HSI14ON; /* (2) */
    while ((RCC->CR2 & RCC_CR2_HSI14RDY) == 0 && ++ctr < 0xfff0){}; /* (3) */
    // calibration
    /* (1) Ensure that ADEN = 0 */
    /* (2) Clear ADEN */
    /* (3) Launch the calibration by setting ADCAL */
    /* (4) Wait until ADCAL=0 */
    if ((ADC1->CR & ADC_CR_ADEN) != 0){ /* (1) */
        ADC1->CR &= (uint32_t)(~ADC_CR_ADEN);  /* (2) */
    }
    ADC1->CR |= ADC_CR_ADCAL; /* (3) */
    ctr = 0; // ADC calibration time is 5.9us
    while ((ADC1->CR & ADC_CR_ADCAL) != 0 && ++ctr < 0xfff0){}; /* (4) */
    // enable ADC
    ctr = 0;
    do{
          ADC1->CR |= ADC_CR_ADEN;
    }while ((ADC1->ISR & ADC_ISR_ADRDY) == 0 && ++ctr < 0xfff0);
    // configure ADC
    /* (1) Select HSI14 by writing 00 in CKMODE (reset value) */
    /* (2) Select the continuous mode */
    /* (3) Select CHSEL0..3 - ADC inputs, 16,17 - t. sensor and vref */
    /* (4) Select a sampling mode of 111 i.e. 239.5 ADC clk to be greater than 17.1us */
    /* (5) Wake-up the VREFINT and Temperature sensor (only for VBAT, Temp sensor and VRefInt) */
    // ADC1->CFGR2 &= ~ADC_CFGR2_CKMODE; /* (1) */
    ADC1->CFGR1 |= ADC_CFGR1_CONT; /* (2)*/
    ADC1->CHSELR = ADC_CHSELR_CHSEL8 | ADC_CHSELR_CHSEL16 | ADC_CHSELR_CHSEL17; /* (3)*/
    ADC1->SMPR |= ADC_SMPR_SMP_0 | ADC_SMPR_SMP_1 | ADC_SMPR_SMP_2; /* (4) */
    ADC->CCR |= ADC_CCR_TSEN | ADC_CCR_VREFEN; /* (5) */
    // configure DMA for ADC
    // DMA for AIN
    /* (1) Enable the peripheral clock on DMA */
    /* (2) Enable DMA transfer on ADC and circular mode */
    /* (3) Configure the peripheral data register address */
    /* (4) Configure the memory address */
    /* (5) Configure the number of DMA tranfer to be performs on DMA channel 1 */
    /* (6) Configure increment, size, interrupts and circular mode */
    /* (7) Enable DMA Channel 1 */
    RCC->AHBENR |= RCC_AHBENR_DMA1EN; /* (1) */
    ADC1->CFGR1 |= ADC_CFGR1_DMAEN | ADC_CFGR1_DMACFG; /* (2) */
    DMA1_Channel1->CPAR = (uint32_t) (&(ADC1->DR)); /* (3) */
    DMA1_Channel1->CMAR = (uint32_t)(ADC_array); /* (4) */
    DMA1_Channel1->CNDTR = NUMBER_OF_ADC_CHANNELS * 9; /* (5) */
    DMA1_Channel1->CCR |= DMA_CCR_MINC | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_CIRC; /* (6) */
    DMA1_Channel1->CCR |= DMA_CCR_EN; /* (7) */
    ADC1->CR |= ADC_CR_ADSTART; /* start the ADC conversions */
}

static inline void pwm_setup(){
    RCC->APB1ENR |= RCC_APB1ENR_TIM14EN; // enable clocking for tim14
    // PWM mode 2
    TIM14->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0;
    TIM14->PSC = 5; // frequency - 8MHz for 31kHz PWM
    // ARR for 8-bit PWM
    TIM14->ARR = 254;
    TIM14->CCR1 = 127; // half light
    TIM14->BDTR |= TIM_BDTR_MOE; // start in OFF state
    TIM14->CCER = TIM_CCER_CC1E; // enable PWM output
    TIM14->CR1 |= TIM_CR1_CEN; // enable timer
}

void hw_setup(){
    gpio_setup();
    adc_setup();
    pwm_setup();
}
