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

#include "adc.h"
#include "effects.h"
#include "hardware.h"
#include "usart.h"

uint32_t sg90step = SG90DEFSTEP;

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
 *      PF1  - open drain       - ext. LED/laser
 *      PA0..PA3 - ADC_IN0..3
 *      PA4 - open drain        - onboard LED (always ON when board works)
 *      PB1, PA6, PA7 - Alt. F. - PWM outputs
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
    // PA6/7 - AF; PB1 - AF
    GPIOA->MODER = GPIO_MODER_MODER6_AF | GPIO_MODER_MODER7_AF |
            GPIO_MODER_MODER0_AI | GPIO_MODER_MODER1_AI |
            GPIO_MODER_MODER2_AI | GPIO_MODER_MODER3_AI |
            GPIO_MODER_MODER4_O;
    GPIOA->OTYPER = GPIO_OTYPER_OT_4;
    GPIOB->MODER = GPIO_MODER_MODER1_AF;
    //RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // enable syscfg clock for EXTI
    GPIOF->ODR = 1<<1;
    GPIOF->MODER = GPIO_MODER_MODER1_O;
    GPIOF->OTYPER = GPIO_OTYPER_OT_1;
    // alternate functions:
    // PA6 - TIM3_CH1, PA7 - TIM3_CH2, PB1 - TIM3_CH4 (all - AF1)
    GPIOA->AFR[0] = (GPIOA->AFR[0] &~ (GPIO_AFRL_AFRL6 | GPIO_AFRL_AFRL7)) \
                | (1 << (6 * 4)) | (1 << (7 * 4));
    GPIOB->AFR[0] = (GPIOB->AFR[0] &~ (GPIO_AFRL_AFRL1)) \
            | (1 << (1 * 4)) ;
}

// change period of PWM
// MAX freq - 200Hz!!!
void setTIM3T(uint32_t T){
    if(T < 1000 || T > 65536) return;
    TIM3->ARR = T - 1;
    // step = ampl / freq(Hz) * 3
    sg90step = SG90_AMPL * T;
    sg90step >>= 18; // /262144
}

static inline void timers_setup(){
    // timer 3 ch1, 2, 4 PWM for three servos
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    // PWM mode 1 (active -> inactive) on all three channels
    TIM3->CCMR1 =   TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 |
                    TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1;
    TIM3->CCMR2 =   TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1;
    // frequency
    TIM3->PSC = 47; // 1MHz -> 1us per tick
    // ARR for 8-bit PWM
    TIM3->ARR = 19999; // 50Hz
    // enable main output
    TIM3->BDTR |= TIM_BDTR_MOE;
    // enable PWM output
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC4E;
    TIM3->DIER = TIM_DIER_UIE; //TIM_DIER_CC1IE | TIM_DIER_CC2IE | TIM_DIER_CC4IE;
    // enable timer & ARR buffering
    TIM3->CR1 |= TIM_CR1_CEN | TIM_CR1_ARPE;
    NVIC_EnableIRQ(TIM3_IRQn);
}

void hw_setup(){
    sysreset();
    gpio_setup();
    adc_setup();
    timers_setup();
    USART1_config();
    iwdg_setup();
}

static uint32_t target_Val[3] = {SG90_MIDPULSE, SG90_MIDPULSE, SG90_MIDPULSE};
static uint32_t target_Speed[3] = {SG90DEFSTEP, SG90DEFSTEP, SG90DEFSTEP};
static uint8_t onpos[3] = {0,0,0};
volatile uint32_t *addr[3] = {&TIM3->CCR1, &TIM3->CCR2, &TIM3->CCR4};

int32_t getPWM(int nch){
    return *addr[nch];
}

// return current value
int32_t setPWM(int nch, uint32_t val, uint32_t speed){
    if(nch < 0 || nch > 2) return 0;
    if(speed > 0){
        if(speed > SG90_STEP) speed = SG90_STEP;
        target_Speed[nch] = speed;
    }
    uint8_t ch = 1;
    if(val >= SG90_MINPULSE && val <= SG90_MAXPULSE) target_Val[nch] = val;
    else if(val == 0) target_Val[nch] = SG90_MINPULSE;
    else if(val == 1) target_Val[nch] = SG90_MAXPULSE;
    else if(val == 2) target_Val[nch] = SG90_MIDPULSE;
    else ch = 0;
    if(ch){
        onpos[nch] = 0;
    }
    return *addr[nch];
}

uint8_t onposition(int nch){
    return onpos[nch];
}

static void chkPWM(int n){
    if(n < 0 || n > 2) return;
    uint32_t cur = *addr[n], tg = target_Val[n];
    if(cur == tg){
        onpos[n] = 1;
        return;
    }
    uint32_t diff = tg - cur;
    int sign = 1;
    if(cur > tg){
        diff = cur - tg;
        sign = -1;
    }
    if(diff > target_Speed[n]) diff = target_Speed[n];
    *addr[n] = cur + sign*diff;
}

void tim3_isr(){
    /*
    if(TIM3->SR & TIM_SR_CC1IF){ // 1st channel
        chkPWM(0);
    }
    if(TIM3->SR & TIM_SR_CC2IF){ // 2nd channel
        chkPWM(1);
    }
    if(TIM3->SR & TIM_SR_CC4IF){ // 3rd channel
        chkPWM(2);
    }*/
    if(TIM3->SR & TIM_SR_UIF){
        chkPWM(0);
        chkPWM(1);
        chkPWM(2);
    }
    TIM3->SR = 0;
}

/*
void exti0_1_isr(){
    if (EXTI->PR & EXTI_PR_PR1){
        EXTI->PR |= EXTI_PR_PR1; // Clear the pending bit
        ;
    }
}
*/
