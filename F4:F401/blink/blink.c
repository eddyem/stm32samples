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

#include <stm32f4.h>

static volatile uint32_t blink_ctr = 0;

void sys_tick_handler(void){
    ++blink_ctr;
}

TRUE_INLINE void gpio_setup(void){
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOCEN;
    GPIOC->MODER = GPIO_MODER_MODER13_O;
    GPIOE->MODER = GPIO_MODER_MODER2_O;
}

int main(void){
    if(!StartHSE()) StartHSI();
    // system frequency is 144MHz
    SysTick_Config((uint32_t)144000); // 1ms
    gpio_setup();
    uint32_t ctr = blink_ctr;
    while(1){
        if(blink_ctr - ctr > 499){
            ctr = blink_ctr;
            pin_toggle(GPIOE, 1<<2);
            pin_toggle(GPIOC, 1<<13);
        }
    }
}
