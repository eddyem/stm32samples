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
#include "adc.h"
#include "hardware.h"
#include "mainloop.h"
#include "protocol.h"
#include "usart.h"
#include <string.h> // memcpy
#include <stm32f0.h>


volatile uint32_t Tms = 0;

// Called when systick fires
void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    char *txt;
    hw_setup();
    SysTick_Config(6000, 1);
    SEND_BLK("Servos controller v0.1\n");
    if(RCC->CSR & RCC_CSR_IWDGRSTF){ // watchdog reset occured
        SEND_BLK("WDGRESET=1");
    }
    if(RCC->CSR & RCC_CSR_SFTRSTF){ // software reset occured
        SEND_BLK("SOFTRESET=1");
    }
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags
    while (1){
        IWDG->KR = IWDG_REFRESH;
        if(usart1_getline(&txt)){ // usart1 received command, process it
            txt = process_command(txt);
        }else txt = NULL;
        if(txt){ // text waits for sending
            while(ALL_OK != usart1_send(txt, 0)){
                IWDG->KR = IWDG_REFRESH;
            }
        }
        mainloop();
        IWDG->KR = IWDG_REFRESH;
        usart1_sendbuf();
    }
}
