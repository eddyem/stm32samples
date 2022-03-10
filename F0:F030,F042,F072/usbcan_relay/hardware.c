/*
 * This file is part of the canrelay project.
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

uint16_t CANID = 0xFFFF; // self CAN ID (read @ init)

// LEDS: 0 - PB12, 1 - PB13, 2 - PB14, 3 - PB15
GPIO_TypeDef *LEDports[LEDSNO] = {GPIOB, GPIOB, GPIOB, GPIOB};
const uint32_t LEDpins[LEDSNO] = {1<<12, 1<<13, 1<<14, 1<<15};
// Buttons: PA2..PA5, pullup
GPIO_TypeDef *BTNports[BTNSNO] = {GPIOA, GPIOA, GPIOA, GPIOA};
const uint32_t BTNpins[BTNSNO] = {1<<2, 1<<3, 1<<4, 1<<5};
// relays: PA0/1
GPIO_TypeDef *R_ports[RelaysNO] = {GPIOA, GPIOA};
const uint32_t R_pins[RelaysNO] = {1<<0, 1<<1};


void gpio_setup(void){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
    // Set LEDS (PB12..15) as output, ADDR (PB0..7) - pullup inputs
    // WARNING! All code here is hardcore!
    pin_set(GPIOB, 0xf<<12); // clear LEDs
    GPIOB->MODER = GPIO_MODER_MODER12_O | GPIO_MODER_MODER13_O | GPIO_MODER_MODER14_O | GPIO_MODER_MODER15_O;
    GPIOB->PUPDR = GPIO_PUPDR0_PU | GPIO_PUPDR1_PU | GPIO_PUPDR2_PU | GPIO_PUPDR3_PU | GPIO_PUPDR4_PU |
                   GPIO_PUPDR5_PU | GPIO_PUPDR6_PU | GPIO_PUPDR7_PU;
    // relays (PA0..1) as outputs, buttons (PA2..5) as pullup inputs
    // PA6,7 - ADC IN, PA8..10 - PWM @ TIM1
    GPIOA->MODER = GPIO_MODER_MODER0_O | GPIO_MODER_MODER1_O | GPIO_MODER_MODER6_AI | GPIO_MODER_MODER7_AI |
                   GPIO_MODER_MODER8_AF | GPIO_MODER_MODER9_AF | GPIO_MODER_MODER10_AF;
    GPIOA->PUPDR = GPIO_PUPDR2_PU | GPIO_PUPDR3_PU | GPIO_PUPDR4_PU | GPIO_PUPDR5_PU;
    CANID = (~READ_INV_CAN_ADDR()) & CAN_INV_ID_MASK;
    // alternate functions @TIM1 PWM (AF2), PA8..10
    GPIOA->AFR[1] = (2 << (0*4)) | (2 << (1*4)) | (2 << (2*4));
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

void tim1_setup(){
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
}

// pause in milliseconds for some purposes
void pause_ms(uint32_t pause){
    uint32_t Tnxt = Tms + pause;
    while(Tms < Tnxt) nop();
}


#ifdef EBUG
void Jump2Boot(){
    __disable_irq();
    IWDG->KR = IWDG_REFRESH;
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
    // enable SYSCFG clocking
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    __DSB();
    // remap system flash memory to 0 (only for STM32F0)
    SYSCFG->CFGR1 = 0x01; __DSB(); __ISB();
    SysMemBootJump = (void (*)(void)) (*((uint32_t *)(addr + 4)));
    // set main stack pointer
    __set_MSP(*((uint32_t *)addr));
    IWDG->KR = IWDG_REFRESH;
    // due to "empty check" mechanism, STM32F042 can jump to bootloader only with empty first 4 bytes of user code
    while ((FLASH->SR & FLASH_SR_BSY) != 0){}
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR;
    if ((FLASH->CR & FLASH_CR_LOCK) != 0){
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = 0x08000000; // erase zero's page
    FLASH->CR |= FLASH_CR_STRT;
    while(!(FLASH->SR & FLASH_SR_EOP));
    FLASH->SR |= FLASH_SR_EOP;
    FLASH->CR &= ~FLASH_CR_PER;
    // jump to bootloader (don't work :( )
    SysMemBootJump();
}
#endif
