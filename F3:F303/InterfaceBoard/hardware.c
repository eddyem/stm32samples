/*
 * This file is part of the multiiface project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

uint8_t Config_mode = 0;

static inline void gpio_setup(){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIODEN
                   | RCC_AHBENR_DMA1EN | RCC_AHBENR_DMA2EN;
    RCC->APB1ENR |= RCC_APB1ENR_CANEN | RCC_APB1ENR_USART2EN | RCC_APB1ENR_USART3EN
                    | RCC_APB1ENR_UART4EN | RCC_APB1ENR_UART5EN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    for(int i = 0; i < 10000; ++i) nop();
/********************************************
* PA1         * USART2 DE   * PP OUT        *
* PA2         * USART2 TX   * AF7           *
* PA3         * USART2 RX   * AF7           *
* PA5         * SPI1 SCK    * AF5           *
* PA6         * SPI1 MISO   * AF5           *
* PA9         * (CONF EN)   * PU IN         *
* PA10        * (USB PU)    * PP OUT        *
* PA11        * USB DM      * AF14          *
* PA12        * USB DP      * AF14          *
* PA13        * SWDIO       * AF0           *
* PA14        * SWCLK       * AF0           *
********************************************/
    GPIOA->AFR[0] = AFRf(7, 2) | AFRf(7, 3) | AFRf(5, 5) | AFRf(5, 6);
    GPIOA->AFR[1] = AFRf(14, 11) | AFRf(14, 12);
    GPIOA->MODER = MODER_O(1) | MODER_AF(2) | MODER_AF(3) | MODER_AF(5) | MODER_AF(6) | MODER_I(9) | MODER_O(10)
                   | MODER_AF(11) | MODER_AF(12) | MODER_AF(13) | MODER_AF(14);
    GPIOA->OSPEEDR = OSPEED_MED(2) | OSPEED_MED(3) | OSPEED_HI(5) | OSPEED_HI(6)
                     | OSPEED_HI(11) | OSPEED_HI(12) | OSPEED_HI(13) | OSPEED_HI(14);
    GPIOA->PUPDR = PUPD_PU(9);
/********************************************
* PB0         * USART1 DE   * PP OUT        *
* PB8         * CAN RX      * AF9           *
* PB9         * CAN TX      * AF9           *
* PB10        * USART3 TX   * AF7           *
* PB11        * USART3 RX   * AF7           *
* PB14        * USART3 DE   * PP OUT        *
********************************************/
    GPIOB->AFR[1] = AFRf(9, 8) | AFRf(9, 9) | AFRf(7, 10) | AFRf(7, 11);
    GPIOB->MODER = MODER_O(0) | MODER_AF(8) | MODER_AF(9) | MODER_AF(10) | MODER_AF(11) | MODER_O(14);
    GPIOB->OSPEEDR = OSPEED_HI(8) | OSPEED_HI(9) | OSPEED_MED(10) | OSPEED_MED(11);
/********************************************
* PC4         * USART1 TX   * AF7           *
* PC5         * USART1 RX   * AF7           *
* PC10        * UART4 TX    * AF5           *
* PC11        * UART4 RX    * AF5           *
* PC12        * UART5 TX    * AF5           *
********************************************/
    GPIOC->AFR[0] = AFRf(7, 4) | AFRf(7, 5);
    GPIOC->AFR[1] = AFRf(5, 10) | AFRf(5, 11) | AFRf(5, 12);
    GPIOC->MODER = MODER_AF(4) | MODER_AF(5) | MODER_AF(10) | MODER_AF(11) | MODER_AF(12);
    GPIOC->OSPEEDR = OSPEED_MED(4) | OSPEED_MED(5) | OSPEED_MED(10) | OSPEED_MED(11) | OSPEED_MED(12);
/********************************************
* PD2         * UART5 RX    * AF5           *
********************************************/
    GPIOD->AFR[0] = AFRf(5, 2);
    GPIOD->MODER = MODER_AF(2);
    GPIOD->OSPEEDR = OSPEED_MED(2);
    for(int i = 0; i < 10000; ++i) nop(); // wait a little before reading CFG pin
    if(CFG_ON()) Config_mode = 1;
}

void hw_setup(){
    gpio_setup();
}

