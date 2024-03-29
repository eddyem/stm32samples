/*
 * This file is part of the F303usartDMA project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "stm32f3.h"
#include "hardware.h"
#include "usart.h"


volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    //sysreset();
    if(!StartHSE()) StartHSI();
    SysTick_Config((uint32_t)72000); // 1ms
    hw_setup();
    usart_setup();
    uint32_t ctr = Tms;
    while(1){
        if(Tms - ctr > 499){
            ctr = Tms;
            pin_toggle(GPIOB, 1 << 1 | 1 << 0); // toggle LED @ PB0
        }
        if(usart_ovr()) usart_send("Buffer overflow occured!\n");
        if(chk_usart()){ // usart1 received data, store in in buffer
            char *txt = NULL;
            int r = usart_getline(&txt);
            if(r){
                txt[r] = 0;
                usart_send("Got data: _");
                usart_send(txt);
                usart_send("_\n");
            }
        }
    }
}
