/*
 * This file is part of the blink project.
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

#include "stm32g0.h"

// KEY (intpullup->0) - PC0
// LED - PC8

static volatile uint32_t blink_ctr = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++blink_ctr;
}

/*
 * Set up timer to fire every x milliseconds
 */
static void systick_setup(uint32_t xms){ // xms < 2098!!!
    static uint32_t curms = 0;
    if(curms == xms) return;
    // 8MHz - HCLK/8
    // this function also clears counter so it starts right away
    SysTick_Config(8000 * xms); // arg should be < 0xffffff, so ms should be < 2098
    curms = xms;
}


static void gpio_setup(void){
    RCC->IOPENR = RCC_IOPENR_GPIOCEN; // enable PC
    // set PC8 as opendrain output, PC0 is pullup input, other as default (AIN)
    GPIOC->MODER = (0xffffffff & ~(GPIO_MODER_MODE8 | GPIO_MODER_MODE0)) | GPIO_MODER_MODER8_O;
    GPIOC->PUPDR = GPIO_PUPDR0_PU; // pullup
    GPIOC->OTYPER = GPIO_OTYPER_OT8; // open drain
}

static const uint32_t L[] = {125,100,125,100,125,200, 350,100,350,100,350,200, 125,100,125,100,125, 1000};

int main(void){
    StartHSE();
    gpio_setup();
    systick_setup(500);
    uint32_t M = 0;
    int pressed = 0;
    /* Do nothing in main loop */
    while (1){
        if(pin_read(GPIOC, 1<<0) == 0){ // key not pressed - 'sos'
            pressed = 1;
            systick_setup(L[M]);
            if(M & 1) pin_set(GPIOC, 1<<8);
            else pin_clear(GPIOC, 1<<8);
            if(++M == 18) M = 0;
        }else{ // key pressed - blink with period of 1s
            if(pressed){
                M = 0;
                pressed = 0;
                systick_setup(500);
            }
            if(blink_ctr & 1) pin_set(GPIOC, 1<<8);
            else pin_clear(GPIOC, 1<<8);
        }
    }
}
