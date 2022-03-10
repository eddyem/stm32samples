/*
 * systick_blink.c
 *
 * Copyright 2017 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 */

#include "stm32f3.h"

static volatile uint32_t blink_ctr = 0;

void sys_tick_handler(void){
    ++blink_ctr;
}

static void gpio_setup(void){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
    GPIOA->MODER = GPIO_MODER_MODER8_O | GPIO_MODER_MODER6_O;
    GPIOA->ODR = 0;
    GPIOB->MODER = GPIO_MODER_MODER0_O | GPIO_MODER_MODER1_O;
    GPIOB->ODR = 3;
}

int main(void){
    if(!StartHSE()) StartHSI();
    SysTick_Config((uint32_t)72000); // 1ms
    gpio_setup();
    //while(blink_ctr < 9);
    uint32_t ctr = blink_ctr;
    while(1){
        if(blink_ctr - ctr > 499){
            ctr = blink_ctr;
            pin_toggle(GPIOA, 1<<8|1<<6);
            pin_toggle(GPIOB, 3);
        }
    }
}
