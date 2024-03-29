/*
 * This file is part of the nitrogen project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "i2c.h"
#include "spi.h"

int LEDsON = 0;

// setup here ALL GPIO pins (due to table in Readme.md)
// leave SWD as default AF; high speed for CLK and some other AF; med speed for some another AF
TRUE_INLINE void gpio_setup(){
    BUZZER_OFF();
    ADCON(0);
    pin_set(LEDs_port, 0xf<<8); // turn off LEDs
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIODEN
                | RCC_AHBENR_GPIOEEN | RCC_AHBENR_GPIOFEN;
    // enable PWM timer TIM3
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    for(int i = 0; i < 10000; ++i) nop();
    // PORT A (PA13/14 - SWDIO/SWCLK - AF0)
    //GPIOA->ODR = 0;
    GPIOA->AFR[0] = 0;
    GPIOA->AFR[1] = AFRf(4, 9) | AFRf(4, 10) | AFRf(14, 11) | AFRf(14,12);
    GPIOA->MODER = MODER_AI(0) | MODER_AI(1) | MODER_AI(2) | MODER_AI(3) | MODER_AI(4) |
                    MODER_AF(9) | MODER_AF(10) | MODER_AF(11) | MODER_AF(12) | MODER_AF(13) | MODER_AF(14);
    GPIOA->OSPEEDR = OSPEED_HI(9) | OSPEED_HI(10) | OSPEED_HI(11) | OSPEED_HI(12) | OSPEED_HI(13) | OSPEED_HI(14);
    GPIOA->OTYPER = 0;
    GPIOA->PUPDR = PUPD_PU(13) | PUPD_PD(14); // SWDIO - pullup, SDCLK - pulldown

    // PORT B
    //GPIOB->ODR = 0;
    GPIOB->AFR[0] = AFRf(4, 6) | AFRf(4, 7);
    GPIOB->AFR[1] = AFRf(5, 13) | AFRf(5, 14) | AFRf(5, 15);
    GPIOB->MODER = MODER_O(0) | MODER_AF(6) | MODER_AF(7) | MODER_O(10) | MODER_O(11) | MODER_O(12) | MODER_AF(13)
                 | MODER_AF(14) | MODER_AF(15);  // 10-DC, 11-RST, 12-LED, 13-SCK, 14-MISO, 15-MOSI
    GPIOB->OSPEEDR = OSPEED_HI(6) | OSPEED_HI(7) | OSPEED_HI(13) | OSPEED_HI(14) | OSPEED_HI(15);
    GPIOB->OTYPER = 0; //OTYPER_OD(15); // MOSI is OD
    GPIOB->PUPDR = 0; //PUPD_PU(14) | PUPD_PU(15); // PU MISO & MOSI

    // PORT C
    //GPIOC->ODR = 0;
    GPIOC->AFR[0] = 0;
    GPIOC->AFR[1] = AFRf(7, 10) | AFRf(7, 11);
    GPIOC->MODER = MODER_AI(0) | MODER_AI(1) | MODER_AI(2) | MODER_AI(3) | MODER_O(9) | MODER_AF(10) | MODER_AF(11);
    GPIOC->OSPEEDR = OSPEED_HI(10);
    GPIOC->OTYPER = 0;
    GPIOC->PUPDR = 0;

    // PORT D
    //GPIOD->ODR = 0;
    GPIOD->AFR[0] = AFRf(7, 0) | AFRf(7, 1) | AFRf(7, 5) | AFRf(7, 6);
    GPIOD->AFR[1] = 0;
    GPIOD->MODER = MODER_AF(0) | MODER_AF(1) | MODER_O(4) | MODER_AF(5) | MODER_AF(6) | MODER_I(9)
                 | MODER_I(10) | MODER_I(11) | MODER_I(12) | MODER_I(13) | MODER_I(14) | MODER_I(15);
    GPIOD->OSPEEDR = OSPEED_HI(1) | OSPEED_HI(5);
    GPIOD->OTYPER = 0;
    GPIOD->PUPDR = PUPD_PU(9) | PUPD_PU(10) | PUPD_PU(11) | PUPD_PU(12) | PUPD_PU(13) | PUPD_PU(14) | PUPD_PU(15);

    // PORT E
    //GPIOE->ODR = 0;
    GPIOE->AFR[0] = AFRf(2, 2) | AFRf(2, 3) | AFRf(2, 4) | AFRf(2, 5);
    GPIOE->AFR[1] = 0;
    GPIOE->MODER = MODER_AF(2) | MODER_AF(3) | MODER_AF(4) | MODER_AF(5) | MODER_O(8) | MODER_O(9) | MODER_O(10) | MODER_O(11);
    GPIOE->OSPEEDR = 0;
    GPIOE->OTYPER = 0;
    GPIOE->PUPDR = 0;

    // PORT F
    //GPIOF->ODR = 0;
    GPIOF->AFR[0] = 0;
    GPIOF->AFR[1] = 0;
    GPIOF->MODER = MODER_AI(2) | MODER_O(10);
    GPIOF->OSPEEDR = 0;
    GPIOF->OTYPER = 0;
    GPIOF->PUPDR = 0;
}

TRUE_INLINE void pwm_setup(){
    TIM3->CR1 = TIM_CR1_ARPE;
    TIM3->PSC = 7199; // 72M/7200 = 10kHz; PWMfreq=10k/100=100Hz
    // PWM mode 1 (active -> inactive)
    TIM3->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1;
    TIM3->CCMR2 = TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1;
    TIM3->CCR1 = 0;
    TIM3->CCR2 = 0;
    TIM3->CCR3 = 0;
    TIM3->CCR4 = 0;
    TIM3->ARR = PWM_CCR_MAX-1; // 8bit PWM
    TIM3->BDTR |= TIM_BDTR_MOE; // enable main output
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;
    TIM3->CR1 |= TIM_CR1_CEN;
}

#ifndef EBUG
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
#endif

void hw_setup(){
    RCC->AHBENR |= RCC_AHBENR_DMA1EN | RCC_AHBENR_DMA2EN;
    gpio_setup();
    IWDG->KR = IWDG_REFRESH;
    i2c_setup(HIGH_SPEED);
    IWDG->KR = IWDG_REFRESH;
    pwm_setup();
}

void setPWM(int nch, uint16_t val){
    switch(nch){
        case 0:
            TIM3->CCR1 = val;
        break;
        case 1:
            TIM3->CCR2 = val;
        break;
        case 2:
            TIM3->CCR3 = val;
        break;
        case 3:
            TIM3->CCR4 = val;
        break;
        default:
        break;
    }
}

uint16_t getPWM(int nch){
    switch(nch){
        case 0:
            return TIM3->CCR1;
        break;
        case 1:
            return TIM3->CCR2;
        break;
        case 2:
            return TIM3->CCR3;
        break;
        case 3:
            return TIM3->CCR4;
        break;
        default:
        break;
    }
    return 0;
}
