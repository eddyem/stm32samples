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

volatile uint32_t Cooler1RPM; // Cooler1 RPM counter by EXTI @PA7
buzzer_state buzzer = BUZZER_OFF; // buzzer state

void adc_setup(){
    uint16_t ctr = 0; // 0xfff0 - more than 1.3ms
    ADC1->CR &= ~ADC_CR_ADEN;
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; // Enable the peripheral clock of the ADC
    RCC->CR2 |= RCC_CR2_HSI14ON; // Start HSI14 RC oscillator
    while ((RCC->CR2 & RCC_CR2_HSI14RDY) == 0 && ++ctr < 0xfff0){}; // Wait HSI14 is ready
    // calibration
    if(ADC1->CR & ADC_CR_ADEN){ // Ensure that ADEN = 0
        ADC1->CR &= (uint32_t)(~ADC_CR_ADEN);  // Clear ADEN
    }
    ADC1->CR |= ADC_CR_ADCAL; // Launch the calibration by setting ADCAL
    ctr = 0; // ADC calibration time is 5.9us
    while(ADC1->CR & ADC_CR_ADCAL && ++ctr < 0xfff0); // Wait until ADCAL=0
    // enable ADC
    ctr = 0;
    do{
          ADC1->CR |= ADC_CR_ADEN;
    }while((ADC1->ISR & ADC_ISR_ADRDY) == 0 && ++ctr < 0xfff0);
    // configure ADC
    ADC1->CFGR1 |= ADC_CFGR1_CONT; // Select the continuous mode
    ADC1->CHSELR =  ADC_CHSELR_CHSEL0 | ADC_CHSELR_CHSEL1 | ADC_CHSELR_CHSEL2 |
                    ADC_CHSELR_CHSEL3 | ADC_CHSELR_CHSEL4 | ADC_CHSELR_CHSEL5 |
                    ADC_CHSELR_CHSEL16 | ADC_CHSELR_CHSEL17; // Select CHSEL0..5 - ADC inputs, 16,17 - t. sensor and vref
    ADC1->SMPR |= ADC_SMPR_SMP; // Select a sampling mode of 111 i.e. 239.5 ADC clk to be greater than 17.1us
    ADC->CCR |= ADC_CCR_TSEN | ADC_CCR_VREFEN; // Wake-up the VREFINT and Temperature sensor
    // configure DMA for ADC
    RCC->AHBENR |= RCC_AHBENR_DMA1EN; // Enable the peripheral clock on DMA
    ADC1->CFGR1 |= ADC_CFGR1_DMAEN | ADC_CFGR1_DMACFG; // Enable DMA transfer on ADC and circular mode
    DMA1_Channel1->CPAR = (uint32_t) (&(ADC1->DR)); // Configure the peripheral data register address
    DMA1_Channel1->CMAR = (uint32_t)(ADC_array); // Configure the memory address
    DMA1_Channel1->CNDTR = NUMBER_OF_ADC_CHANNELS * 9; // Configure the number of DMA tranfer to be performs on DMA channel 1
    DMA1_Channel1->CCR |= DMA_CCR_MINC | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_CIRC; // Configure increment, size, interrupts and circular mode
    DMA1_Channel1->CCR |= DMA_CCR_EN; // Enable DMA Channel 1
    ADC1->CR |= ADC_CR_ADSTART; // start the ADC conversions
}


static inline void gpio_setup(void){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN;
    OFF(BUZZER); OFF(RELAY); OFF(COOLER0); OFF(COOLER1);
    // All outputs are pullups
    // PA0..5 - ADC, PA6, PA8..10 - timers (PA6, PA7 - pullup, PA7 - exti), PA14 - buzzer
    GPIOA->MODER =  GPIO_MODER_MODER0_AI | GPIO_MODER_MODER1_AI |
                    GPIO_MODER_MODER2_AI | GPIO_MODER_MODER3_AI |
                    GPIO_MODER_MODER4_AI | GPIO_MODER_MODER5_AI |
                    GPIO_MODER_MODER6_AF | GPIO_MODER_MODER8_AF |
                    GPIO_MODER_MODER9_AF | GPIO_MODER_MODER10_AF|
                    GPIO_MODER_MODER14_O;
    GPIOA->PUPDR = GPIO_PUPDR6_PU | GPIO_PUPDR7_PU;
    // EXTI @ PA7
    SYSCFG->EXTICR[1] = SYSCFG_EXTICR2_EXTI7_PA; // PORTA for EXTI
    EXTI->IMR = EXTI_IMR_MR7; // select pin 7
    EXTI->RTSR = EXTI_RTSR_TR7; // rising edge @ pin 7
    NVIC_EnableIRQ(EXTI4_15_IRQn); // enable interrupt
    NVIC_SetPriority(EXTI4_15_IRQn, 4); // set low priority
    // PB4/5 - cooler, PB14/15 - buttons (pullup)
    GPIOB->MODER = GPIO_MODER_MODER4_O | GPIO_MODER_MODER5_O;
    GPIOB->PUPDR = GPIO_PUPDR14_PU | GPIO_PUPDR15_PU;
    // PC13 - relay
    GPIOC->MODER = GPIO_MODER_MODER13_O;
    /* Alternate functions:
     * PA6  - TIM3_CH1  AF1
     * PA8  - TIM1_CH1  AF2
     * PA9  - TIM1_CH2  AF2
     * PA10 - TIM1_CH3  AF2
     */
    GPIOA->AFR[0] = (1 << (6*4));
    GPIOA->AFR[1] = (2 << (0*4)) | (2 << (1*4)) | (2 << (2*4));
}

static inline void timers_setup(){
    // TIM1 channels 1..3 - PWM output
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; // enable clocking
    TIM1->PSC = 19; // F=48/20 = 2.4MHz
    TIM1->ARR = 100; // PWM frequency = 2.4/101 = 23.76kHz
    // PWM mode 1 (OCxM = 110), preload enable
    TIM1->CCMR1 =   TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE |
                    TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2PE;
    TIM1->CCMR2 =   TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3PE;
    TIM1->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E; // active high (CC1P=0), enable outputs
    TIM1->BDTR |= TIM_BDTR_MOE; // enable main output
    TIM1->CR1 |= TIM_CR1_CEN; // enable timer
    TIM1->EGR |= TIM_EGR_UG; // force update generation
    // TIM3 channel 1 - external counter on channel1
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    TIM3->SMCR = TIM_SMCR_TS_2 | TIM_SMCR_TS_0 | TIM_SMCR_SMS; // TS=101, SMS=111 - external trigger on input1
    TIM3->CCMR1 = TIM_CCMR1_CC1S_0; // CC1 is input mapped on channel TI1
    TIM3->CR1 = TIM_CR1_CEN;
}

void HW_setup(){
    gpio_setup();
    adc_setup();
    timers_setup();
}

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

// pause in milliseconds for some purposes
void pause_ms(uint32_t pause){
    uint32_t Tnxt = Tms + pause;
    while(Tms < Tnxt) nop();
}

void Jump2Boot(){
    void (*SysMemBootJump)(void);
    volatile uint32_t addr = SystemMem;
    // reset systick
    SysTick->CTRL = 0;
    // reset clocks

    RCC->APB1RSTR = RCC_APB1RSTR_CECRST    | RCC_APB1RSTR_DACRST    | RCC_APB1RSTR_PWRRST    | RCC_APB1RSTR_CRSRST  |
                    RCC_APB1RSTR_CANRST    | RCC_APB1RSTR_USBRST    | RCC_APB1RSTR_I2C2RST   | RCC_APB1RSTR_I2C1RST |
                    RCC_APB1RSTR_USART4RST | RCC_APB1RSTR_USART3RST | RCC_APB1RSTR_USART2RST | RCC_APB1RSTR_SPI2RST |
                    RCC_APB1RSTR_WWDGRST   | RCC_APB1RSTR_TIM14RST  |
#ifdef STM32F072xB
            RCC_APB1RSTR_TIM7RST   | RCC_APB1RSTR_TIM6RST |
#endif
                    RCC_APB1RSTR_TIM3RST   | RCC_APB1RSTR_TIM2RST;
    RCC->APB2RSTR = RCC_APB2RSTR_DBGMCURST | RCC_APB2RSTR_TIM17RST | RCC_APB2RSTR_TIM16RST |
#ifdef STM32F072xB
            RCC_APB2RSTR_TIM15RST |
#endif
                    RCC_APB2RSTR_USART1RST | RCC_APB2RSTR_SPI1RST  | RCC_APB2RSTR_TIM1RST  | RCC_APB2RSTR_ADCRST   | RCC_APB2RSTR_SYSCFGRST;
    RCC->AHBRSTR = 0;
    RCC->APB1RSTR = 0;
    RCC->APB2RSTR = 0;
    // remap memory to 0 (only for STM32F0)
    SYSCFG->CFGR1 = 0x01; __DSB(); __ISB();
    SysMemBootJump = (void (*)(void)) (*((uint32_t *)(addr + 4)));
    // set main stack pointer
    __set_MSP(*((uint32_t *)addr));
    // jump to bootloader
    SysMemBootJump();
}

void buzzer_chk(){ // check buzzer state
    static uint32_t lastTms = 0;
    static buzzer_state oldstate = BUZZER_OFF;
    uint32_t B, S; // length of beep and silence
    if(buzzer == oldstate){ // keep current state
        if(lastTms > Tms) return;
        switch(buzzer){ // beep on/off
            case BUZZER_LONG:
                B = LONG_BUZZER_PERIOD;
                S = LONG_BUZZER_PAUSE;
                SEND("long ");
            break;
            case BUZZER_SHORT:
                B = SHORT_BUZZER_PERIOD;
                S = SHORT_BUZZER_PAUSE;
                SEND("short ");
            break;
            default:
            return;
        }
        if(CHK(BUZZER)){ // is ON
            SEND("1->0\n");
            OFF(BUZZER);
            lastTms = Tms + S;
        }else{ // is OFF
            SEND("0->1\n");
            ON(BUZZER);
            lastTms = Tms + B;
        }
        sendbuf();
        return;
    }
    SEND("New state: ");
    switch(buzzer){ // change buzzer state
        case BUZZER_ON:
            ON(BUZZER);
            SEND("on");
        break;
        case BUZZER_OFF:
            OFF(BUZZER);
            SEND("off");
        break;
        case BUZZER_LONG:
            ON(BUZZER);
            lastTms = Tms + LONG_BUZZER_PERIOD;
            SEND("long");
        break;
        case BUZZER_SHORT:
            ON(BUZZER);
            lastTms = Tms + SHORT_BUZZER_PERIOD;
            SEND("short");
        break;
    }
    newline(); sendbuf();
    oldstate = buzzer;
}

void exti4_15_isr(){
    EXTI->PR |= EXTI_PR_PR7; // clear pending bit
    ++Cooler1RPM;
}
