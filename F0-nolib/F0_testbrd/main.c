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

#include "adc.h"
#include "hardware.h"
#include "usart.h"
#include "proto.h"
#include "usb.h"
#include "usb_lib.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    uint32_t lastT = 0;
    sysreset();
    SysTick_Config(6000, 1);
    hw_setup();
    usart_setup();

    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags

    USB_setup();
    iwdg_setup();

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(lastT > Tms || Tms - lastT > 499){
            lastT = Tms;
            transmit_tbuf(); // non-blocking transmission of data from UART buffer every 0.5s
        }
        usb_proc();
        char *txt;
        if((txt = get_USB())){
            const char *ans = parse_cmd(txt);
            if(ans){
                uint16_t l = 0; const char *p = ans;
                while(*p++) l++;
                USB_send((uint8_t*)ans, l);
                if(ans[l-1] != '\n') USND("\n");
            }
        }
        for(int n = 1; n <= USARTNUM; ++n){
            if(usartrx(n)){ // usart1 received data, store in in buffer
                int r = usart_getline(n, &txt);
                if(r){
                    txt[r] = 0;
                    USND("Got string over USART"); USB_sendstr(u2str(n));
                    USND(":\n"); USB_sendstr(txt);
                }
            }
        }
    }
    return 0;
}

