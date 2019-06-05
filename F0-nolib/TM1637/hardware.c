/*
 * This file is part of the TM1637 project.
 * Copyright 2019 Edward Emelianov <eddy@sao.ru>.
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


/**
 * @brief gpio_setup - setup GPIOs for external IO
 * GPIO pinout:
 *      PC13 - open drain       - onboard LED (blinks when board works)
 */
static inline void gpio_setup(){
    // Enable clocks to the GPIO subsystems
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN;
    GPIOC->MODER = GPIO_MODER_MODER13_O;
    GPIOC->OTYPER = GPIO_OTYPER_OT_13;
    /*
    // PA6/7 - AF; PB1 - AF
    GPIOA->MODER = GPIO_MODER_MODER4_O | GPIO_MODER_MODER6_AF | GPIO_MODER_MODER7_AF;
    GPIOA->OTYPER = GPIO_OTYPER_OT_4;
    // alternate functions:
    // PA6 - TIM3_CH1, PA7 - TIM3_CH2
    GPIOA->AFR[0] = (GPIOA->AFR[0] &~ (GPIO_AFRL_AFRL6 | GPIO_AFRL_AFRL7)) \
                | (1 << (6 * 4)) | (1 << (7 * 4));
    */
}

static inline void i2c_setup(){
    I2C1->CR1 = 0;
/*
 * GPIO Resources: I2C1_SCL - PB6 (AF1), I2C1_SDA - PB7 (AF1)
 */
    GPIOB->MODER |= GPIO_MODER_MODER6_AF | GPIO_MODER_MODER7_AF; // alternate function
    GPIOB->AFR[0] = (GPIOB->AFR[0] &~ (GPIO_AFRL_AFRL6 | GPIO_AFRL_AFRL7)) \
                | (1 << (6 * 4)) | (1 << (7 * 4));
   // GPIOB->OTYPER |= GPIO_OTYPER_OT_6 | GPIO_OTYPER_OT_7; // opendrain
    // I2C
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; // timing
    RCC->CFGR3 |= RCC_CFGR3_I2C1SW; // use sysclock for timing
    // 100kHz
    // Clock = 6MHz, 0.16(6)us, need 5us (*30)
    // PRESC=4 (f/5), SCLDEL=0 (t_SU=5/6us), SDADEL=0 (t_HD=5/6us), SCLL,SCLH=14 (2.(3)us)
    I2C1->TIMINGR = (0xB<<28) | (4<<20) | (2<<16) | (0x12<<8) | (0x11);
    I2C1->CR1 = I2C_CR1_PE; // Enable I2C
}

void hw_setup(){
    sysreset();
    gpio_setup();
    i2c_setup();
    USART1_config();
}

