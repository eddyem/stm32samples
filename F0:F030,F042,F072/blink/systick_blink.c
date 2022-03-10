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

#include "stm32f0.h"

static volatile uint32_t blink_ctr = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++blink_ctr;
}

/*
 * Set up timer to fire every x milliseconds
 */
static void systick_setup(uint32_t xms){
    static uint32_t curms = 0;
    if(curms == xms) return;
    // 6MHz - HCLK/8 (due to HPRE=1 HCLK=SYSCLK)
    // this function also clears counter so it starts right away
    SysTick_Config(6000 * xms, 1);
    curms = xms;
}


/* set STM32 to clock by 48MHz from HSI oscillator */
//static void clock_setup(void){
    //StartHSE(RCC_CR_CSSON | RCC_CR_HSEBYP);
//}

static void gpio_setup(void){
    /* Enable clocks to the GPIO subsystems (A&B) */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
    /* Set green led (PB3) as output */
    GPIOB->MODER = GPIO_MODER_MODER3_O;
   // gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);
    /* D2 - PA12 - switch blink rate (pullup input) */
    GPIOA->PUPDR = GPIO_PUPDR_PUPDR12_0;
    //gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO12);
}

static const uint32_t L[] = {125,100,125,100,125,200, 350,100,350,100,350,200, 125,100,125,100,125, 1000};

int main(void){
    sysreset();
    // set divider by 8
    SysTick->CTRL &= ~SysTick_CTRL_CLKSOURCE_Msk;
    //StartHSE();
    //StartHSI48();
  //  clock_setup();
    gpio_setup();

    /* 500ms ticks =>  1000ms period => 1Hz blinks */
    systick_setup(100);
    /* Do nothing in main loop */
    while (1){
        if(pin_read(GPIOA, 1<<12)){
            uint32_t T = blink_ctr % 18;
            systick_setup(L[T]);
            if(T & 1) pin_clear(GPIOB, 1<<3);
            else pin_set(GPIOB, 1<<3);
        }else{
            systick_setup(2000);
            if(blink_ctr & 1) pin_clear(GPIOB, 1<<3);
            else pin_set(GPIOB, 1<<3);

        }
    }
}
