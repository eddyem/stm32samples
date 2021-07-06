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
#include "proto.h"
#include "spi.h"
#include "usart.h"
#include "usb.h"

void iwdg_setup(){
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

static inline void gpio_setup(){
    // here we turn on clocking for all periph.
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_DMAEN;
    // Set LEDS (PA6-8, PB0/1) as Out & AF (PWM); PA0,1,5 - AIN, PA4 - DAC
    GPIOA->MODER = GPIO_MODER_MODER0_AI | GPIO_MODER_MODER1_AI | GPIO_MODER_MODER4_AI |
            GPIO_MODER_MODER5_AI | GPIO_MODER_MODER6_AF | GPIO_MODER_MODER7_AF | GPIO_MODER_MODER8_AF;
    GPIOB->MODER = GPIO_MODER_MODER0_AF | GPIO_MODER_MODER1_AF;
    // alternate functions: PA6-8: TIM3CH1,2 and TIM1_CH1 (AF1, AF1, AF2)
    //                      PB0-1: TIM3CH3,4 (AF1, AF1),
    GPIOA->AFR[0] = (1 << (6 * 4)) | (1 << (7 * 4));
    GPIOA->AFR[1] = (2 << (0 * 4));
    GPIOB->AFR[0] = (1 << (0 * 4)) | (1 << (1 * 4));
}

// Setup ADC and DAC
static inline void adc_setup(){
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
    // configure ADC to generate a triangle wave on DAC1_OUT synchronized by TIM6 HW trigger
    /* (1) Select HSI14 by writing 00 in CKMODE (reset value) */
    /* (2) Select the continuous mode */
    /* (3) Select CHSEL0,1,5 - ADC inputs, 16,17 - t. sensor and vref */
    /* (4) Select a sampling mode of 111 i.e. 239.5 ADC clk to be greater than 17.1us */
    /* (5) Wake-up the VREFINT and Temperature sensor (only for VBAT, Temp sensor and VRefInt) */
    // ADC1->CFGR2 &= ~ADC_CFGR2_CKMODE; /* (1) */
    ADC1->CFGR1 |= ADC_CFGR1_CONT; /* (2)*/
    ADC1->CHSELR = ADC_CHSELR_CHSEL0 | ADC_CHSELR_CHSEL1 | ADC_CHSELR_CHSEL5 | ADC_CHSELR_CHSEL16 | ADC_CHSELR_CHSEL17; /* (3)*/
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
    // DAC
    /* (1) Enable the peripheral clock of the DAC */
    /* (2) Configure WAVEx at 10,
           Configure mask amplitude for ch1 (MAMP1) at 1011 for a 4095-bits amplitude
           enable the DAC ch1, disable buffer on ch1,
           and select TIM6 as trigger by keeping 000 in TSEL1 */
    RCC->APB1ENR |= RCC_APB1ENR_DACEN; /* (1) */
    DAC->CR |= DAC_CR_WAVE1_1
             | DAC_CR_MAMP1_3 | DAC_CR_MAMP1_1 | DAC_CR_MAMP1_0
             | DAC_CR_BOFF1 | DAC_CR_TEN1 | DAC_CR_EN1; /* (2) */
    // configure the Timer 6 to generate an external trigger on TRGO each microsecond
    /* (1) Enable the peripheral clock of the TIM6 */
    /* (2) Configure MMS=010 to output a rising edge at each update event */
    /* (3) Select PCLK/2 i.e. 48MHz/2=24MHz */
    /* (4) Set one update event each 1 microsecond */
    /* (5) Enable TIM6 */
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN; /* (1) */
    TIM6->CR2 |= TIM_CR2_MMS_1; /* (2) */
    TIM6->PSC = 1; /* (3) */
    TIM6->ARR = (uint16_t)24; /* (4) */
    TIM6->CR1 |= TIM_CR1_CEN; /* (5) */
}

static inline void pwm_setup(){
    // enable clocking for tim1 & tim3
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    // PWM mode 2
    TIM1->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0;
    TIM3->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0 |
                  TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_0;
    TIM3->CCMR2 = TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_0 |
                  TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_0;
    // frequency - 8MHz for 31kHz PWM
    TIM1->PSC = 5;
    TIM3->PSC = 5;
    // ARR for 8-bit PWM
    TIM1->ARR = 254;
    TIM3->ARR = 254;
    TIM1->CCR1 = 127;
    TIM3->CCR1 = 63; TIM3->CCR2 = 127; TIM3->CCR3 = 191; TIM3->CCR4 = 250;
    // enable main output
    TIM1->BDTR |= TIM_BDTR_MOE;
    TIM3->BDTR |= TIM_BDTR_MOE;
    // enable PWM outputs
    TIM1->CCER = TIM_CCER_CC1E;
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;
    // start timers
    TIM1->CR1 |= TIM_CR1_CEN;
    TIM3->CR1 |= TIM_CR1_CEN;
}

void hw_setup(){
    gpio_setup();
    adc_setup();
    pwm_setup();
}

// USART & SPI both use common DMA interrupts, so put them together here
// SPI Rx use the same DMA channels as USART Tx, so they can't work together!
// USART1 Tx (channel 2) & SPI1 Tx (channel 3)
void dma1_channel2_3_isr(){
    if(DMA1->ISR & DMA_ISR_TCIF2){ // Tx
        DMA1->IFCR |= DMA_IFCR_CTCIF2; // clear TC flag
        txrdy[0] = 1;
    }
    if(DMA1->ISR & DMA_ISR_TCIF3){ // transfer done
        DMA1->IFCR |= DMA_IFCR_CTCIF3;
        SPI_status[0] = SPI_READY;
        USND("SPI1 tx done\n");
    }
    if(DMA1->ISR & DMA_ISR_TEIF2){ // receiver overflow
        DMA1->IFCR |= DMA_IFCR_CTEIF2;
        SPIoverfl[0] = 1;
    }
}
// USART2 + USART3 Tx (channels 4 and 7) & SPI2 Tx (channel 5)
void dma1_channel4_5_isr(){
    if(DMA1->ISR & DMA_ISR_TCIF4){ // Tx
        DMA1->IFCR |= DMA_IFCR_CTCIF4; // clear TC flag
        txrdy[1] = 1;
    }
    if(DMA1->ISR & DMA_ISR_TEIF4){
        DMA1->IFCR |= DMA_IFCR_CTEIF4;
        SPIoverfl[1] = 1;
    }
    if(DMA1->ISR & DMA_ISR_TCIF5){
        DMA1->IFCR |= DMA_IFCR_CTCIF5;
        SPI_status[1] = SPI_READY;
        USND("SPI2 tx done\n");
    }
#ifdef USART3
    if(DMA1->ISR & DMA_ISR_TCIF7){ // Tx
        DMA1->IFCR |= DMA_IFCR_CTCIF7; // clear TC flag
        txrdy[2] = 1;
    }
#endif
}
