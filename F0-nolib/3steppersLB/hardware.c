/*
 * This file is part of the 3steppers project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "can.h"

// Buttons: PA10, PA13, PA14, PA15, pullup (0 active)
volatile GPIO_TypeDef *BTNports[BTNSNO] = {GPIOA, GPIOA, GPIOA, GPIOA};
const uint32_t BTNpins[BTNSNO] = {1<<10, 1<<13, 1<<14, 1<<15};
// Limit switches: PC13, PC14, PC15, pulldown (0 active)
volatile GPIO_TypeDef *ESWports[ESWNO] = {GPIOC, GPIOC, GPIOC};
const uint32_t ESWpins[ESWNO] = {1<<13, 1<<14, 1<<15};
// external GPIO
volatile GPIO_TypeDef *EXTports[EXTNO] = {GPIOB, GPIOB, GPIOB};
const uint32_t EXTpins[EXTNO] = {1<<13, 1<<14, 1<<15};
// motors: DIR/EN
volatile GPIO_TypeDef *ENports[MOTORSNO] = {GPIOB, GPIOB, GPIOB};
const uint32_t ENpins[MOTORSNO] = {1<<0, 1<<2, 1<<11};
volatile GPIO_TypeDef *DIRports[MOTORSNO] = {GPIOB, GPIOB, GPIOB};
const uint32_t DIRpins[MOTORSNO] = {1<<1, 1<<10, 1<<12};

void gpio_setup(void){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIOFEN;
    GPIOA->MODER = GPIO_MODER_MODER0_AF | GPIO_MODER_MODER1_AF | GPIO_MODER_MODER2_AF | GPIO_MODER_MODER3_AI |
                   GPIO_MODER_MODER4_AF | GPIO_MODER_MODER5_AI | GPIO_MODER_MODER6_AF | GPIO_MODER_MODER7_AF |
                   GPIO_MODER_MODER8_AF | GPIO_MODER_MODER9_AF;
    GPIOA->PUPDR = GPIO_PUPDR10_PU | GPIO_PUPDR13_PU | GPIO_PUPDR14_PU | GPIO_PUPDR15_PU;
    GPIOA->AFR[0] = (2 << (0*4)) | (2 << (1*4)) | (0 << (2*4)) | (4 << (4*4)) | (5 << (6*4)) | (5 << (7*4));
    GPIOA->AFR[1] = (2 << (0*4)) | (2 << (1*4));
    pin_set(GPIOB, (1<<0) | (1<<2) | (1<<11)); // turn off motors @ start
    GPIOB->MODER = GPIO_MODER_MODER0_O  | GPIO_MODER_MODER1_O  | GPIO_MODER_MODER2_O  | GPIO_MODER_MODER3_O  |
                   GPIO_MODER_MODER4_AF | GPIO_MODER_MODER5_AF | GPIO_MODER_MODER6_AF | GPIO_MODER_MODER7_AF |
                   GPIO_MODER_MODER10_O | GPIO_MODER_MODER11_O | GPIO_MODER_MODER12_O | GPIO_MODER_MODER13_O |
                   GPIO_MODER_MODER14_O | GPIO_MODER_MODER15_O ;
    GPIOB->AFR[0] = (1 << (4*4)) | (1 << (5*4)) | (1 << (6*4)) | (1 << (7*4));
    GPIOC->PUPDR = GPIO_PUPDR13_PD | GPIO_PUPDR14_PD | GPIO_PUPDR15_PD;
    GPIOF->MODER = GPIO_MODER_MODER0_O;
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

void timers_setup(){
#if 0
    // TIM1 channels 1..3 - PWM output
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; // enable clocking
    TIM1->PSC = 9; // F=48/10 = 4.8MHz
    TIM1->ARR = 255; // PWM frequency = 4800/256 = 18.75kHz
    // PWM mode 1 (OCxM = 110), preload enable
    TIM1->CCMR1 =   TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE |
                    TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2PE;
    TIM1->CCMR2 =   TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3PE;
    TIM1->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E; // active high (CC1P=0), enable outputs
    TIM1->BDTR |= TIM_BDTR_MOE; // enable main output
    TIM1->CR1 |= TIM_CR1_CEN; // enable timer
    TIM1->EGR |= TIM_EGR_UG; // force update generation
#endif
}

// pause in milliseconds for some purposes
void pause_ms(uint32_t pause){
    uint32_t Tnxt = Tms + pause;
    while(Tms < Tnxt) nop();
}

void Jump2Boot(){ // for STM32F072
    void (*SysMemBootJump)(void);
    volatile uint32_t addr = 0x1FFFC800;
    // reset systick
    SysTick->CTRL = 0;
    // reset clocks
    RCC->APB1RSTR = RCC_APB1RSTR_CECRST    | RCC_APB1RSTR_DACRST    | RCC_APB1RSTR_PWRRST    | RCC_APB1RSTR_CRSRST  |
                    RCC_APB1RSTR_CANRST    | RCC_APB1RSTR_USBRST    | RCC_APB1RSTR_I2C2RST   | RCC_APB1RSTR_I2C1RST |
                    RCC_APB1RSTR_USART4RST | RCC_APB1RSTR_USART3RST | RCC_APB1RSTR_USART2RST | RCC_APB1RSTR_SPI2RST |
                    RCC_APB1RSTR_WWDGRST   | RCC_APB1RSTR_TIM14RST  | RCC_APB1RSTR_TIM7RST   | RCC_APB1RSTR_TIM6RST |
                    RCC_APB1RSTR_TIM3RST   | RCC_APB1RSTR_TIM2RST;
    RCC->APB2RSTR = RCC_APB2RSTR_DBGMCURST | RCC_APB2RSTR_TIM17RST | RCC_APB2RSTR_TIM16RST | RCC_APB2RSTR_TIM15RST |
                    RCC_APB2RSTR_USART1RST | RCC_APB2RSTR_SPI1RST  | RCC_APB2RSTR_TIM1RST  | RCC_APB2RSTR_ADCRST   | RCC_APB2RSTR_SYSCFGRST;
    RCC->AHBRSTR = 0;
    RCC->APB1RSTR = 0;
    RCC->APB2RSTR = 0;
    // Enable the SYSCFG peripheral.
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    // remap memory to 0 (only for STM32F0)
    SYSCFG->CFGR1 = 0x01; __DSB(); __ISB();
    SysMemBootJump = (void (*)(void)) (*((uint32_t *)(addr + 4)));
    // set main stack pointer
    __set_MSP(*((uint32_t *)addr));
    // jump to bootloader
    SysMemBootJump();
}

