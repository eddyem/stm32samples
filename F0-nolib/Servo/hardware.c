/*
 * This file is part of the Chiller project.
 * Copyright 2018 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "usart.h"
#include "adc.h"

static inline void iwdg_setup(){
    /* Enable the peripheral clock RTC */
    /* (1) Enable the LSI (40kHz) */
    /* (2) Wait while it is not ready */
    RCC->CSR |= RCC_CSR_LSION; /* (1) */
    while((RCC->CSR & RCC_CSR_LSIRDY) != RCC_CSR_LSIRDY); /* (2) */
    /* Configure IWDG */
    /* (1) Activate IWDG (not needed if done in option bytes) */
    /* (2) Enable write access to IWDG registers */
    /* (3) Set prescaler by 64 (1.6ms for each tick) */
    /* (4) Set reload value to have a rollover each 1s */
    /* (5) Check if flags are reset */
    /* (6) Refresh counter */
    IWDG->KR = IWDG_START; /* (1) */
    IWDG->KR = IWDG_WRITE_ACCESS; /* (2) */
    IWDG->PR = IWDG_PR_PR_1; /* (3) */
    IWDG->RLR = 625; /* (4) */
    while(IWDG->SR); /* (5) */
    IWDG->KR = IWDG_REFRESH; /* (6) */
}

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
    // configure ADC
    /* (1) Select HSI14 by writing 00 in CKMODE (reset value) */
    /* (2) Select the continuous mode */
    /* (3) Select CHSEL0..3 - ADC inputs, 16,17 - t. sensor and vref */
    /* (4) Select a sampling mode of 111 i.e. 239.5 ADC clk to be greater than 17.1us */
    /* (5) Wake-up the VREFINT and Temperature sensor (only for VBAT, Temp sensor and VRefInt) */
    // ADC1->CFGR2 &= ~ADC_CFGR2_CKMODE; /* (1) */
    ADC1->CFGR1 |= ADC_CFGR1_CONT; /* (2)*/
    ADC1->CHSELR = ADC_CHSELR_CHSEL0 | ADC_CHSELR_CHSEL1 | ADC_CHSELR_CHSEL2 |
            ADC_CHSELR_CHSEL3 | ADC_CHSELR_CHSEL16 | ADC_CHSELR_CHSEL17; /* (3)*/
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

/**
 * @brief gpio_setup - setup GPIOs for external IO
 * GPIO pinout:
 *      PA5  - floating input   - Ef of TLE5205
 *      PA13 - open drain       - IN1 of TLE5205
 *      PA14 - open drain       - IN2 of TLE5205
 *      PF0  - floating input   - water level alert
 *      PF1  - push-pull        - external alarm
 *      PA0..PA3 - ADC_IN0..3
 *      PA4, PA6, PA7 - PWM outputs
 * Registers
 *      MODER  - input/output/alternate/analog (2 bit)
 *      OTYPER - 0 pushpull, 1 opendrain
 *      OSPEEDR - x0 low, 01 medium, 11 high
 *      PUPDR - no/pullup/pulldown/resr (2 bit)
 *      IDR - input
 *      ODR - output
 *      BSRR - 0..15 - set, 16..31 - reset
 *      BRR - 0..15 - reset
 *      AFRL/AFRH - alternate functions (4 bit, AF: 0..7); L - AFR0..7, H - ARF8..15
 */
static inline void gpio_setup(){
    // Enable clocks to the GPIO subsystems
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOFEN;
    GPIOA->MODER =
            GPIO_MODER_MODER13_O | GPIO_MODER_MODER14_O |
            GPIO_MODER_MODER4_AF | GPIO_MODER_MODER6_AF |
            GPIO_MODER_MODER7_AF |
            GPIO_MODER_MODER0_AI | GPIO_MODER_MODER1_AI |
            GPIO_MODER_MODER2_AI | GPIO_MODER_MODER3_AI;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // enable syscfg clock for EXTI
    GPIOA->OTYPER = 3 << 13; // 13/14 opendrain
    GPIOF->MODER = GPIO_MODER_MODER1_O;
    // PB1 - interrupt input
    /* (2) Select Port B for pin 1 external interrupt by writing 0001 in EXTI1*/
    /* (3) Configure the corresponding mask bit in the EXTI_IMR register */
    /* (4) Configure the Trigger Selection bits of the Interrupt line on rising edge*/
    /* (5) Configure the Trigger Selection bits of the Interrupt line on falling edge*/
    SYSCFG->EXTICR[0] = SYSCFG_EXTICR1_EXTI1_PB; /* (2) */
    EXTI->IMR = EXTI_IMR_MR1; /* (3) */
    //EXTI->RTSR = 0x0000; /* (4) */
    EXTI->FTSR = EXTI_FTSR_TR1; /* (5) */
    /* (6) Enable Interrupt on EXTI0_1 */
    /* (7) Set priority for EXTI0_1 */
    NVIC_EnableIRQ(EXTI0_1_IRQn); /* (6) */
    NVIC_SetPriority(EXTI0_1_IRQn, 3); /* (7) */
    // alternate functions:
    // PA4 - TIM14_CH1 (AF4)
    // PA6 - TIM16_CH1 (AF5), PA7 - TIM17_CH1 (AF5)
    GPIOA->AFR[0] = (GPIOA->AFR[0] &~ (GPIO_AFRL_AFRL4 | GPIO_AFRL_AFRL6 | GPIO_AFRL_AFRL7)) \
                | (4 << (4 * 4)) | (5 << (6 * 4)) | (5 << (7 * 4));
}

static inline void timers_setup(){
    // timer 14 ch1 - cooler PWM
    // timer 16 ch1 - heater PWM
    // timer 17 ch1 - pump PWM
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM14EN; // enable clocking for timers 3 and 14
    RCC->APB2ENR |= RCC_APB2ENR_TIM16EN | RCC_APB2ENR_TIM17EN; // & timers 16/17
    // PWM mode 1 (active -> inactive)
    TIM14->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1;
    TIM16->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1;
    TIM17->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1;
    // frequency
    TIM14->PSC = 59; // 0.8MHz for 3kHz PWM
    TIM16->PSC = 18749; // 2.56kHz for 10Hz PWM
    TIM17->PSC = 5; // 8MHz for 31kHz PWM
    // ARR for 8-bit PWM
    TIM14->ARR = 254;
    TIM16->ARR = 254;
    TIM17->ARR = 254;
    // start in OFF state
    // TIM14->CCR1 = 0; and so on
    // enable main output
    TIM14->BDTR |= TIM_BDTR_MOE;
    TIM16->BDTR |= TIM_BDTR_MOE;
    TIM17->BDTR |= TIM_BDTR_MOE;
    // enable PWM output
    TIM14->CCER = TIM_CCER_CC1E;
    TIM16->CCER = TIM_CCER_CC1E;
    TIM17->CCER = TIM_CCER_CC1E;
    // enable timers
    TIM14->CR1 |= TIM_CR1_CEN;
    TIM16->CR1 |= TIM_CR1_CEN;
    TIM17->CR1 |= TIM_CR1_CEN;
}

void hw_setup(){
    sysreset();
    gpio_setup();
    adc_setup();
    timers_setup();
    USART1_config();
    iwdg_setup();
}


void exti0_1_isr(){
    if (EXTI->PR & EXTI_PR_PR1){
        EXTI->PR |= EXTI_PR_PR1; /* Clear the pending bit */
        ;
    }
}
