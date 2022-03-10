/*
 * main.c
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
#include "usart.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

static void gpio_setup(void){
    /* Enable clocks to the GPIO subsystems (A&B) */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    /* Set green leds (PA5 & PA4) as output */
    GPIOA->MODER = GPIO_MODER_MODER4_O | GPIO_MODER_MODER5_O;
    GPIOA->OTYPER = 1 << 5; // PA5 - opendrain, PA4 - pushpull
    /* PA6 - switch blink rate (pullup input) */
    GPIOA->PUPDR = GPIO_PUPDR_PUPDR6_0;
}

/**
 * reverce line
 */
char *rline(char *in, int L){
    static char out[UARTBUFSZ];
    if(L > UARTBUFSZ - 1) return in;
    char *optr = out, *iptr = &in[L-1];
    for(; L > 0; --L) *optr++ = *iptr--;
    *optr = '\n';
    return out;
}

int main(void){
    uint32_t lastT = 0;
    int L = 0;
    char *txt;
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup();
    USART1_config();
    //StartHSE();
    pin_set(GPIOA, 1<<5); // clear extern LED
    /* Do nothing in main loop */
    while (1){
        if(lastT > Tms || Tms - lastT > 499){
            pin_toggle(GPIOA, 1<<4); // blink by onboard LED once per second
            lastT = Tms;
        }
        if(usart1rx()){ // usart1 received data, store in in buffer
            L = usart1_getline(&txt);
            if(!pin_read(GPIOA, 1<<6)){ // there's a jumper: reverse string
                txt = rline(txt, L);
            }
            pin_clear(GPIOA, 1<<5); // set extern LED
        }
        if(L){ // text waits for sending
            if(ALL_OK == usart1_send(txt, L)){
                L = 0;
                pin_set(GPIOA, 1<<5); // clear extern LED
            }
        }
    }
}
