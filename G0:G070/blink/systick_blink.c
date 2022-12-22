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

#include "stm32g0.h"

// KEY (intpullup->0) - PC0
// LED - PC7

static volatile uint32_t blink_ctr = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++blink_ctr;
}

/*
 * Set up timer to fire every x milliseconds
 */
static void systick_setup(uint32_t xms){ // xms < 1864!!!
    static uint32_t curms = 0;
    if(curms == xms) return;
    // 9MHz - HCLK/8
    // this function also clears counter so it starts right away
    SysTick_Config(9000 * xms); // arg should be < 0xffffff, so ms should be < 1864
    curms = xms;
}


static void gpio_setup(void){
    RCC->IOPENR = RCC_IOPENR_GPIOCEN; // enable PC
    // set PC7 as opendrain output, PC0 is pullup input
    GPIOC->MODER = GPIO_MODER_MODER7_O;
    GPIOC->PUPDR = GPIO_PUPDR_PUPD0;
    GPIOC->OTYPER = GPIO_OTYPER_OT7;
}

static const uint32_t L[] = {125,100,125,100,125,200, 350,100,350,100,350,200, 125,100,125,100,125, 1000};

int main(void){
    StartHSE();
    //StartHSI48();
    gpio_setup();
    /* 500ms ticks =>  1000ms period => 1Hz blinks */
    systick_setup(100);
    /* Do nothing in main loop */
    while (1){
        if(pin_read(GPIOC, 1<<7)){ // key not pressed - 'sos'
            uint32_t T = blink_ctr % 18;
            systick_setup(L[T]);
            if(T & 1) pin_clear(GPIOC, 1<<7);
            else pin_set(GPIOC, 1<<7);
        }else{ // key pressed - blink with period of 1s
            systick_setup(500);
            if(blink_ctr & 1) pin_clear(GPIOC, 1<<7);
            else pin_set(GPIOC, 1<<7);

        }
    }
}
