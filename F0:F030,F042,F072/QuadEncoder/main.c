/*
 * main.c
 *
 * Copyright 2018 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#include "hardware.h"
#include "protocol.h"
#include "usart.h"
#include <stm32f0.h>


volatile uint32_t Tms = 0;

// Called when systick fires
void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    uint32_t T = 0;
    uint32_t tim3cnt = 0;
    char *txt;
    hw_setup();
    SysTick_Config(6000, 1);
    SEND("Encoder controller v0.1\n");
    while (1){
        if(usart1_getline(&txt)){ // usart1 received command, process it
            txt = process_command(txt);
        }else txt = NULL;
        if(txt){ // text waits for sending
            while(ALL_OK != usart1_send(txt, 0));
        }
        if(Tms - T == 10){
            T = Tms;
            if(tim3cnt != TIM3->CNT){
                int32_t diff = TIM3->CNT - tim3cnt;
                if(tim3upd){
                    if(TIM3->CR1 & TIM_CR1_DIR) diff -= 80;
                    else diff += 80;
                    tim3upd = 0;
                }
                tim3cnt = TIM3->CNT;
                put_string("Speed: ");
                put_int(diff);
                put_string(", pos: ");
                put_uint(TIM3->CNT);
                put_char('\n');
            }
        }
        usart1_sendbuf();
    }
}
