/*
 * This file is part of the usart project.
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

#include <stm32g0.h>
#include "usart.h"

// KEY (intpullup->0) - PC0
// LED - PC8

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

static void gpio_setup(void){
    RCC->IOPENR |= RCC_IOPENR_GPIOCEN; // enable PC
    // set PC8 as opendrain output, PC0 is pullup input, other as default (AIN)
    GPIOC->MODER = (0xffffffff & ~(GPIO_MODER_MODE8 | GPIO_MODER_MODE0)) | GPIO_MODER_MODER8_O;
    GPIOC->PUPDR = GPIO_PUPDR0_PU; // pullup
    GPIOC->OTYPER = GPIO_OTYPER_OT8; // open drain
}

int main(void){
    StartHSE();
    SysTick_Config(8000); // 1ms counter
    gpio_setup();
    usart3_setup();
    uint32_t T = 0;
    int sent = 0;
    /* Do nothing in main loop */
    while (1){
        if(Tms - T > 499){ // blink LED
            T = Tms;
            pin_toggle(GPIOC, 1<<8);
            usart3_sendbuf();
        }
        if(pin_read(GPIOC, 1<<0) == 0){ // key pressed - send data over USART
            if(!sent){
                usart3_sendstr("Button pressed\n");
                sent = 1;
            }
        }else sent = 0;
        int wasbo = 0;
        char *rcv = usart3_getline(&wasbo);
        if(wasbo) usart3_sendstr("Buffer overflow occured @ last message\n");
        if(rcv){
            usart3_sendstr("\nI got: ");
            usart3_sendstr(rcv);
            usart3_send("\n\n", 2);
        }
    }
}
