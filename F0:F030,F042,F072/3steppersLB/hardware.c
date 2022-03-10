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
#include "steppers.h"
#include "strfunct.h"

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

// timers for motors
volatile TIM_TypeDef *mottimers[MOTORSNO] = {TIM15, TIM14, TIM16};
// timers for encoders
volatile TIM_TypeDef *enctimers[MOTORSNO] = {TIM1, TIM2, TIM3};

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

static IRQn_Type motirqs[MOTORSNO] = {TIM15_IRQn, TIM14_IRQn, TIM16_IRQn};
// motor's PWM
static void setup_mpwm(int i){
    volatile TIM_TypeDef *TIM = mottimers[i];
    TIM->CR1 = TIM_CR1_ARPE; // buffered ARR
    TIM->PSC = MOTORTIM_PSC; // 16MHz
    // PWM mode 1 (active -> inactive)
    TIM->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1;
#if MOTORTIM_ARRMIN < 5
#error "change the code!"
#endif
    TIM->CCR1 = MOTORTIM_ARRMIN - 3; // ~10us for pulse duration
    TIM->ARR = 0xffff;
//    TIM->EGR = TIM_EGR_UG; // generate update to refresh ARR
    TIM->BDTR |= TIM_BDTR_MOE; // enable main output
    TIM->CCER = TIM_CCER_CC1E; // turn it on, active high
    TIM->DIER = TIM_DIER_CC1IE; // allow CC interrupt (we should count steps)
    NVIC_EnableIRQ(motirqs[i]);
}

static IRQn_Type encirqs[MOTORSNO] = {TIM1_BRK_UP_TRG_COM_IRQn, TIM2_IRQn, TIM3_IRQn};
static void setup_enc(int i){
    volatile TIM_TypeDef *TIM = enctimers[i];
    /* (1) Configure TI1FP1 on TI1 (CC1S = 01)
           configure TI1FP2 on TI2 (CC2S = 01)
           filters sampling = fDTS/8, N=6 */
    /* (2) Configure TI1FP1 and TI1FP2 non inverted (CC1P = CC2P = 0, reset value) */
    /* (3) Configure both inputs are active on both rising and falling edges
          (SMS = 011), set external trigger filter to f_DTS/8, N=6 (ETF=1000) */
    /* (4) Enable the counter by writing CEN=1 in the TIMx_CR1 register,
           set tDTS=4*tCK_INT. */
    TIM->CCMR1 = TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0 | TIM_CCMR1_IC1F_3 | TIM_CCMR1_IC2F_3; /* (1)*/
    //TIMx->CCER &= (uint16_t)(~(TIM_CCER_CC21 | TIM_CCER_CC2P); /* (2) */
    TIM->SMCR =  TIM_SMCR_ETF_3 | TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1; /* (3) */
    // enable update interrupt
    TIM->DIER = TIM_DIER_UIE;
    // generate interrupt each revolution
    TIM->ARR = the_conf.encrev[i];
    // enable timer
    TIM->CR1 = TIM_CR1_CKD_1 | TIM_CR1_CEN; /* (4) */
    NVIC_EnableIRQ(encirqs[i]);
}

// timers 14,15,16,17 - PWM@ch1, 1,2,3 - encoders @ ch1/2
void timers_setup(){
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM14EN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_TIM15EN | RCC_APB2ENR_TIM16EN | RCC_APB2ENR_TIM17EN; // enable clocking
    // setup PWM @ TIM17 - single PWM channel
    // PWM mode 1 (active -> inactive)
    TIM17->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1;
    TIM17->PSC = 5; // 8MHz for 31kHz PWM
    TIM17->ARR = 254; // ARR for 8-bit PWM
    TIM17->CCR1 = 0; // level=0
    TIM17->BDTR |= TIM_BDTR_MOE; // enable main output
    TIM17->CCER = TIM_CCER_CC1E; // enable PWM output
    TIM17->CR1 = TIM_CR1_CEN; // enable timer
    for(int i = 0; i < MOTORSNO; ++i)
        setup_mpwm(i);
    for(int i = 0; i < MOTORSNO; ++i){
        if(the_conf.motflags[i].haveencoder){ // motor have the encoder
            setup_enc(i);
        }
    }
}

// pause in milliseconds for some purposes
void pause_ms(uint32_t pause){
    uint32_t Tnxt = Tms + pause;
    while(Tms < Tnxt) nop();
}

#ifndef STM32F072xB
#warning "Only F072 can jump to bootloader"
#endif

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

// calculate MSB position of value `val`
//                             0 1 2 3 4 5 6 7 8 9 a b c d e f
static const uint8_t bval[] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3};
uint8_t MSB(uint16_t val){
    register uint8_t r = 0;
    if(val & 0xff00){r += 8; val >>= 8;}
    if(val & 0x00f0){r += 4; val >>= 4;}
    return ((uint8_t)r + bval[val]);
}

// state 1 - pressed, 0 - released (pin active is zero)
uint8_t ESW_state(uint8_t x){
    uint8_t val = ((ESWports[x]->IDR & ESWpins[x]) ? 0 : 1);
    if(the_conf.motflags[x].eswinv) val = !val;
    return val;
}

void tim14_isr(){
    //TIM14->CR1 &= ~TIM_CR1_CEN;
    addmicrostep(1);
    TIM14->SR = 0;
}
void tim15_isr(){
    //TIM15->CR1 &= ~TIM_CR1_CEN;
    addmicrostep(0);
    TIM15->SR = 0;
}
void tim16_isr(){
    //TIM16->CR1 &= ~TIM_CR1_CEN;
    addmicrostep(2);
    TIM16->SR = 0;
}


void tim1_brk_up_trg_com_isr(){
    encoders_UPD(0);
}
void tim2_isr(){
    encoders_UPD(1);
}
void tim3_isr(){
    encoders_UPD(2);
}
