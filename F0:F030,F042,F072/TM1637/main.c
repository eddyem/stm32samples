/*
 * This file is part of the TM1637 project.
 * Copyright 2019 Edward Emelianov <eddy@sao.ru>.
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
#include "i2c.h"
#include "protocol.h"
#include "usart.h"
#include <stm32f0.h>


volatile uint32_t Tms = 0;

// Called when systick fires
void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    uint32_t T = 0, TLED = 0;
    char *txt;
    hw_setup();
    SysTick_Config(6000, 1);
    SEND("LED display controller v0.1\n");
    while (1){
        if(Tms - TLED > 499){
            TLED = Tms;
            pin_toggle(GPIOC, 1<<13);
        }
        if(usart1_getline(&txt)){ // usart1 received command, process it
            txt = process_command(txt);
        }else txt = NULL;
        if(txt){ // text waits for sending
            while(ALL_OK != usart1_send(txt, 0));
        }
        if(Tms - T > 49){
            T = Tms;
        }
        usart1_sendbuf();
    }
}
