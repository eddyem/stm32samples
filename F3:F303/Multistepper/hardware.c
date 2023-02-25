/*
 * This file is part of the multistepper project.
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

#include "flash.h"
#include "hardware.h"
#include "steppers.h"

// Buttons: PA9, PA10, PF6, PD3, PD4, PD5, pullup (active - 0)
volatile GPIO_TypeDef* const BTNports[BTNSNO] = {GPIOA, GPIOA, GPIOF, GPIOD, GPIOD, GPIOD};
const uint32_t BTNpins[BTNSNO] = {1<<9, 1<<10, 1<<6, 1<<3, 1<<4, 1<<5};

// Limit switches: 0:PC14/PC13, 1:PB9/PB7, 2:PD7/PD6, 3:PC10/PC11, 4:PC7/PD15,
//   5:PD11/PD10, 6:PE11/PE10, 7:PE7/PE8
volatile GPIO_TypeDef *ESWports[MOTORSNO][ESWNO] = {
    {GPIOC, GPIOC}, // 0
    {GPIOB, GPIOB},
    {GPIOD, GPIOD}, // 2
    {GPIOC, GPIOC},
    {GPIOC, GPIOD}, // 4
    {GPIOD, GPIOD},
    {GPIOE, GPIOE}, // 6
    {GPIOE, GPIOE},
};
const uint32_t ESWpins[MOTORSNO][ESWNO] = {
    {1<<14, 1<<13},
    {1<<9, 1<<7}, // 1
    {1<<7, 1<<6},
    {1<<10, 1<<11}, // 3
    {1<<7, 1<<15},
    {1<<11, 1<<10}, // 5
    {1<<11, 1<<10},
    {1<<7, 1<<8}, // 7
};

// GPIO pins: PB11, PE15, PE14
volatile GPIO_TypeDef *EXTports[EXTNO] = {
    GPIOB, GPIOE, GPIOE
};
const uint32_t EXTpins[EXTNO] = {
    1<<11, 1<<15, 1<<14
};

// motors: DIR/EN
// EN: 0:PF10, 1:PE1, 2:PB6, 3:PD2, 4:PC9, 5:PD14, 6:PE13, 7:PB1
volatile GPIO_TypeDef *ENports[MOTORSNO] = {
    GPIOF, GPIOE, GPIOB, GPIOD, GPIOC, GPIOD, GPIOE, GPIOB};
const uint32_t ENpins[MOTORSNO] = {
    1<<10, 1<<1, 1<<6, 1<<2, 1<<9, 1<<14, 1<<13, 1<<1};
// DIR: 0:PC15, 1:PE0, 2:PB4, 3:PC12, 4:PC8, 5:PD13, 6:PE12, 7:PB2
volatile GPIO_TypeDef *DIRports[MOTORSNO] = {
    GPIOC, GPIOE, GPIOB, GPIOC, GPIOC, GPIOD, GPIOE, GPIOB};
const uint32_t DIRpins[MOTORSNO] = {
    1<<15, 1<<0, 1<<4, 1<<12, 1<<8, 1<<13, 1<<12, 1<<2};
// timers for motors: 0:t15c1, 1:t16c1, 2:t17c1, 3:t2ch1, 4:t8ch1, 5:t4c1, 6:t1c1, 7:t3c3
volatile TIM_TypeDef *mottimers[MOTORSNO] = {
    TIM15, TIM16, TIM17, TIM2, TIM8, TIM4, TIM1, TIM3};
const uint8_t mottchannels[MOTORSNO] = {1,1,1,1,1,1,1,3};
static IRQn_Type motirqs[MOTORSNO] = {
    TIM15_IRQn, TIM16_IRQn, TIM17_IRQn, TIM2_IRQn, TIM8_CC_IRQn, TIM4_IRQn, TIM1_CC_IRQn, TIM3_IRQn};

// return two bits: 0 - ESW0, 1 - ESW1 (if inactive -> 1; if active -> 0)
uint8_t ESW_state(uint8_t MOTno){
    uint8_t val = 0;
    if(the_conf.isSPI){ // only ESW0 used
        val = ((ESWports[MOTno][0]->IDR & ESWpins[MOTno][0]) ? 1 : 0);
        if(the_conf.motflags[MOTno].eswinv) val ^= 1;
        return val;
    }else for(int i = 0; i < 2; ++i){
        val |= ((ESWports[MOTno][i]->IDR & ESWpins[MOTno][i]) ? 1 : 0) << i;
    }
    if(the_conf.motflags[MOTno].eswinv) val ^= 3;
    return val;
}

// calculate MSB position of value `val`
//                             0 1 2 3 4 5 6 7 8 9 a b c d e f
static const uint8_t bval[] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3};
uint8_t MSB(uint16_t val){
    register uint8_t r = 0;
    if(val & 0xff00){r += 8; val >>= 8;}
    if(val & 0x00f0){r += 4; val >>= 4;}
    return ((uint8_t)r + bval[val]);
}

// setup here ALL GPIO pins (due to table in Readme.md)
// leave SWD as default AF; high speed for CLK and some other AF; med speed for some another AF
TRUE_INLINE void gpio_setup(){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIODEN
                | RCC_AHBENR_GPIOEEN | RCC_AHBENR_GPIOFEN;
    // enable timers: 1,2,3,4,8,15,16,17
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM4EN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_TIM8EN | RCC_APB2ENR_TIM15EN | RCC_APB2ENR_TIM16EN | RCC_APB2ENR_TIM17EN;
    for(int i = 0; i < 10000; ++i) nop();
    GPIOA->ODR = 0;
    GPIOA->AFR[0] = AFRf(5, 5) | AFRf(5, 6) | AFRf(5, 7);
    GPIOA->AFR[1] = /*AFRf(4, 10) |*/ /*AFRf(14, 11) | AFRf(14,12) |*/ AFRf(1, 15);
    GPIOA->MODER = MODER_AF(5) | MODER_AF(6) | MODER_AF(7) | MODER_O(8) | MODER_I(9) | MODER_I(10) /*MODER_AF(10)*/
                 | /*MODER_AF(11) | MODER_AF(12) |*/ MODER_AF(13) | MODER_AF(14) | MODER_AF(15);
    GPIOA->OSPEEDR = OSPEED_MED(5) | OSPEED_MED(6) | OSPEED_MED(7) | OSPEED_HI(11) | OSPEED_HI(12)
                 | OSPEED_HI(13) | OSPEED_HI(15);
    GPIOA->OTYPER = 0;
    GPIOA->PUPDR = PUPD_PU(8) | PUPD_PU(9) | PUPD_PU(10);

    GPIOB->ODR = 0;
    GPIOB->AFR[0] = AFRf(2, 0) | AFRf(7, 3) | AFRf(10, 5);
    GPIOB->AFR[1] = AFRf(1, 8) | AFRf(7, 10) | AFRf(5, 13) | AFRf(5, 14) | AFRf(5, 15);
    GPIOB->MODER = MODER_AF(0) | MODER_O(1) | MODER_O(2) | MODER_AF(3) | MODER_O(4) | MODER_AF(5) | MODER_O(6)
                 | MODER_I(7) | MODER_AF(8) | MODER_I(9) | MODER_AF(10) | MODER_I(11) | MODER_O(12) | MODER_AF(13)
                 | MODER_AF(14) | MODER_AF(15);
    GPIOB->OSPEEDR = OSPEED_HI(0) | OSPEED_HI(5) | OSPEED_HI(8) | OSPEED_MED(13) | OSPEED_MED(14) | OSPEED_MED(15);
    GPIOB->OTYPER = 0;
    // USART2_Tx (PB3) and USART3_Tx (PB10) are also pullup
    GPIOB->PUPDR = PUPD_PU(3) | PUPD_PU(7) | PUPD_PU(9) | PUPD_PU(7) | PUPD_PU(11);

    GPIOC->ODR = 0;
    GPIOC->AFR[0] = AFRf(7, 4) | AFRf(7, 5) | AFRf(4, 6);
    GPIOC->MODER = MODER_O(0) | MODER_O(1) | MODER_O(2) | MODER_O(3) | MODER_AF(4) | MODER_AF(5) | MODER_AF(6)
                 | MODER_I(7) | MODER_O(8) | MODER_O(9) | MODER_I(10) | MODER_I(11) | MODER_O(12) | MODER_I(13)
                 | MODER_I(14) | MODER_O(15);
    GPIOC->OSPEEDR = OSPEED_HI(6);
    GPIOC->OTYPER = 0;
    GPIOC->PUPDR = PUPD_PU(7) | PUPD_PU(10) | PUPD_PU(11) | PUPD_PU(13) | PUPD_PU(14);

    GPIOD->ODR = 0;
    GPIOD->AFR[0] = AFRf(7, 0) | AFRf(7, 1);
    GPIOD->AFR[1] = AFRf(2, 12);
    GPIOD->MODER = MODER_AF(0) | MODER_AF(1) | MODER_O(2) | MODER_I(3) | MODER_I(4) | MODER_I(5) | MODER_I(6)
                 | MODER_I(7) | MODER_O(8) | MODER_O(9) | MODER_I(10) | MODER_I(11) | MODER_AF(12) | MODER_O(13)
                 | MODER_O(14) | MODER_I(15);
    GPIOD->OSPEEDR = OSPEED_HI(0) | OSPEED_HI(1) | OSPEED_HI(12);
    GPIOD->OTYPER = 0;
    GPIOD->PUPDR = PUPD_PU(3) | PUPD_PU(4) | PUPD_PU(5) | PUPD_PU(6) | PUPD_PU(7) | PUPD_PU(10) | PUPD_PU(11)
                 | PUPD_PU(15);

    GPIOE->ODR = 0;
    GPIOE->AFR[1] = AFRf(2, 9);
    GPIOE->MODER = MODER_O(0) | MODER_O(1) | MODER_I(2) | MODER_O(3) | MODER_O(4) | MODER_O(5) | MODER_O(6)
                 | MODER_I(7) | MODER_I(8) | MODER_AF(9) | MODER_I(10) | MODER_I(11) | MODER_O(12) | MODER_O(13)
                 | MODER_I(14) | MODER_I(15);
    GPIOE->OSPEEDR = OSPEED_HI(9);
    GPIOE->OTYPER = 0;
    GPIOE->PUPDR = PUPD_PU(2) | PUPD_PU(7) | PUPD_PU(8) | PUPD_PU(10) | PUPD_PU(11) | PUPD_PU(14) | PUPD_PU(15);

    GPIOF->ODR = 0;
    /*GPIOF->AFR[0] = AFRf(4, 6);*/
    GPIOF->AFR[1] = AFRf(3, 9);
    GPIOF->MODER = MODER_I(6) | MODER_AF(9) | MODER_O(10);
    GPIOF->OSPEEDR = OSPEED_HI(9);
    GPIOF->OTYPER = 0;
    GPIOF->PUPDR =  PUPD_PU(6);
}

#ifndef EBUG
TRUE_INLINE void iwdg_setup(){
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

// motor's PWM
static void setup_mpwm(int i){
    volatile TIM_TypeDef *TIM = mottimers[i];
    TIM->CR1 = TIM_CR1_ARPE; // buffered ARR
    TIM->PSC = MOTORTIM_PSC; // 16MHz
    // PWM mode 1 (active -> inactive)
    uint8_t n = mottchannels[i];
    switch(n){
        case 1:
            TIM->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1;
        break;
        case 2:
            TIM->CCMR1 = TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1;
        break;
        case 3:
            TIM->CCMR2 = TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1;
        break;
        default:
            TIM->CCMR2 = TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1;
    }
#if MOTORTIM_ARRMIN < 5
#error "change the code!"
#endif
    TIM->CCR1 = MOTORTIM_ARRMIN - 3; // ~10us for pulse duration
    TIM->ARR = 0xffff;
//    TIM->EGR = TIM_EGR_UG; // generate update to refresh ARR
    TIM->BDTR |= TIM_BDTR_MOE; // enable main output
    TIM->CCER = 1<<((n-1)*4); // turn it on, active high
    TIM->DIER = 1<<n; // allow CC interrupt (we should count steps)
    NVIC_EnableIRQ(motirqs[i]);
}


void mottimers_setup(){
    for(int i = 0; i < MOTORSNO; ++i) setup_mpwm(i);
}

void hw_setup(){
    gpio_setup();
    mottimers_setup();
#ifndef EBUG
    iwdg_setup();
#endif
}


// timers for motors: 0:t15c1, 1:t16c1, 2:t17c1, 3:t2ch1, 4:t8ch1, 5:t4c1, 6:t1c1, 7:t3c3
void tim1_cc_isr(){
    addmicrostep(6);
    TIM1->SR = 0;
}
void tim2_isr(){
    addmicrostep(3);
    TIM2->SR = 0;
}
void tim3_isr(){
    addmicrostep(7);
    TIM3->SR = 0;
}
void tim4_isr(){
    addmicrostep(5);
    TIM4->SR = 0;
}
void tim8_cc_isr(){
    addmicrostep(4);
    TIM8->SR = 0;
}
void tim1_brk_tim15_isr(){
    addmicrostep(0);
    TIM15->SR = 0;
}
void tim1_up_tim16_isr(){
    addmicrostep(1);
    TIM16->SR = 0;
}
void tim1_trg_com_tim17_isr(){
    addmicrostep(2);
    TIM17->SR = 0;
}
