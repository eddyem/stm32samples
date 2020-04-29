/*
 * This file is part of the Stepper project.
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

TIM_TypeDef *TIMx = TIM15; // stepper's timer

static uint8_t brdADDR = 0;

void Jump2Boot(){
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
    // remap memory to 0 (only for STM32F0)
    SYSCFG->CFGR1 = 0x01; __DSB(); __ISB();
    SysMemBootJump = (void (*)(void)) (*((uint32_t *)(addr + 4)));
    // set main stack pointer
    __set_MSP(*((uint32_t *)addr));
    // jump to bootloader
    SysMemBootJump();
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

/*
MODER   - input/output/alternate/analog
OTYPER  - pushpull/opendrain
OSPEEDR - low(x0)/med(01)/high(11)
PUPDR   - no/pullup/pulldown/reserved
AFRL, AFRH - alternate fno
*/
void gpio_setup(){
    // here we turn on clocking for all GPIO used
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIOFEN
                   | RCC_AHBENR_DMAEN;
    // setup pins need @start: Vio_ON (PF0, opendrain), ~FAULT (PF1, floating IN),
    //  ~SLEEP (PC15, pushpull), DIR (PA4, pushpull), ~EN (PC13, pushpull)
    // ~CS, microstepping2, (PC14, pushpull)
    // PA8 - Tx/Rx
    // PB12..15 - board address, pullup input; PB0..2, PB10 - ESW, pullup inputs (inverse)
    VIO_OFF();
    SLEEP_ON();
    DRV_DISABLE();
    // PA. PP: PA4, PA8; AIN: PA0, PA1
    GPIOA->MODER = GPIO_MODER_MODER8_O | GPIO_MODER_MODER4_O | GPIO_MODER_MODER1_AI | GPIO_MODER_MODER0_AI;
    GPIOA->PUPDR = 0;
    GPIOA->OTYPER = 0;
    // PB. PUin: 0,1,2,10,12,13,14,15
    GPIOB->MODER = 0;
    GPIOB->PUPDR = GPIO_PUPDR0_PU | GPIO_PUPDR1_PU | GPIO_PUPDR2_PU | GPIO_PUPDR10_PU |
            GPIO_PUPDR12_PU | GPIO_PUPDR13_PU | GPIO_PUPDR14_PU | GPIO_PUPDR15_PU;
    GPIOB->OTYPER = 0;
    // PC. PP: 13..15
    GPIOC->MODER = GPIO_MODER_MODER13_O | GPIO_MODER_MODER14_O | GPIO_MODER_MODER15_O;
    GPIOC->PUPDR = 0;
    GPIOC->OTYPER = 0;
    // PF. OD: 0; FLin: 1
    GPIOF->MODER = GPIO_MODER_MODER0_O;
    //GPIOF->PUPDR = GPIO_PUPDR1_PU;
    GPIOF->PUPDR = 0;
    GPIOF->OTYPER = 0;
    // other pins will be set up later
/*
    // Set LEDS (PC13/14) as output
    GPIOC->MODER = (GPIOC->MODER & ~(GPIO_MODER_MODER13  | GPIO_MODER_MODER14)
                    ) |
                    GPIO_MODER_MODER13_O | GPIO_MODER_MODER14_O;
*/
    brdADDR = READ_BRD_ADDR();
}

// PA3 (STEP): TIM15_CH2; 48MHz -> 48kHz
void timer_setup(){
    RCC->APB2ENR |= RCC_APB2ENR_TIM15EN; // enable clocking
    TIM15->CR1 &= ~TIM_CR1_CEN; // turn off timer
    TIM15->CCMR1 = TIM_CCMR1_OC2M_2; // Force inactive
    TIM15->PSC = 999;
    TIM15->CCER = TIM_CCER_CC2E;
    TIM15->CCR1 = 1; // very short pulse
    TIM15->ARR = 1000;
    // enable IRQ & update values
    TIM15->EGR = TIM_EGR_UG;
    TIM15->DIER = TIM_DIER_CC2IE;
    NVIC_EnableIRQ(TIM15_IRQn);
    NVIC_SetPriority(TIM15_IRQn, 0);
}

uint8_t refreshBRDaddr(){
    return (brdADDR = READ_BRD_ADDR());
}
uint8_t getBRDaddr(){return brdADDR;}

void sleep(uint16_t ms){
    uint32_t Tnew = Tms + ms;
    while(Tnew != Tms) nop();
}
