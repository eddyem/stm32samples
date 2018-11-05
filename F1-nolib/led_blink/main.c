/*
 * main.c
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include "stm32f1.h"

static volatile uint32_t blink_ctr = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++blink_ctr;
}

// SysTick is 24 bit counter, so 16777215 - max value!!!
// After HSE is ON it works @9MHz (????)
static void systick_setup(uint32_t xms){
    static uint32_t curms = 0;
    if(curms == xms) return;
    // this function also clears counter so it starts right away
    SysTick_Config(72000 * xms);
    curms = xms;
}

static const uint16_t L[] = {125,100,125,100,125,200, 60,100,60,100,60,200, 125,100,125,100,125, 230};

static void gpio_setup(void){
    /* Enable clocks to the GPIO subsystems (A&B) */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;
    /* Set leds (PB8/PB9) as opendrain output */
    GPIOB->CRH = CRH(8, CNF_ODOUTPUT|MODE_SLOW) | CRH(9, CNF_ODOUTPUT|MODE_SLOW);
    /* Set buttons (PC0/1) as inputs with weak pullups */
    GPIOC->ODR = 3; // pullups for PC0/1
    GPIOC->CRL = CRL(0, CNF_PUDINPUT|MODE_INPUT) | CRL(1, CNF_PUDINPUT|MODE_INPUT);
}

int main(){
    sysreset();
    StartHSE();
    gpio_setup();
    systick_setup(100);
    uint32_t oldctr = 0xff;
    pin_clear(GPIOB, 3<<8);
    /* Do nothing in main loop */
    while (1){
        if(pin_read(GPIOC, 1) && pin_read(GPIOC, 2)){ // no buttons present - morze @ LED1 (PB9)
            if(oldctr != blink_ctr){
                uint32_t T = blink_ctr % 18;
                systick_setup(L[T]);
                if(T & 1) pin_set(GPIOB, 1<<9);
                else pin_clear(GPIOB, 1<<9);
                oldctr = blink_ctr;
            }
        }else{ // button pressed: turn ON given LED
            if(pin_read(GPIOC, 1) == 0){ // PC0 pressed (button S2)
                systick_setup(5);
                if(blink_ctr - oldctr > 499){
                    pin_toggle(GPIOB, 1<<9);
                    oldctr = blink_ctr;
                }
            }else pin_set(GPIOB, 1<<9);
            if(pin_read(GPIOC, 2) == 0){ // PC1 pressed (button S3)
                systick_setup(1);
                if(blink_ctr - oldctr > 499){
                    pin_toggle(GPIOB, 1<<8);
                    oldctr = blink_ctr;
                }
            }else pin_set(GPIOB, 1<<8);

        }
    }
}

