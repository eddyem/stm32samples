/*
 * This file is part of the pl2303 project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "proto.h"
#include "usart.h"
#include "usb.h"

#define MAXSTRLEN    RBINSZ

const char *test = "123456789A123456789B123456789C123456789D123456789E123456789F123456789G123456789H123456789I123456789J123456789K123456789L123456789M123456789N123456789O123456789P123456789Q123456789R123456789S123456789T123456789U123456789V123456789W123456789X123456789Y\n"
        "123456789A123456789B123456789C123456789D123456789E123456789F123456789G123456789H123456789I123456789J123456789K123456789L123456789M123456789N123456789O123456789P123456789Q123456789R123456789S123456789T123456789U123456789V123456789W123456789X123456789Y\n";

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    char inbuff[MAXSTRLEN+1];
    int hse = 0;
    if(StartHSE()){
        hse = 1;
        SysTick_Config((uint32_t)72000); // 1ms
    }else{
        StartHSI();
        SysTick_Config((uint32_t)48000); // 1ms
    }
    hw_setup();
    usart_setup();
    USB_setup();
    if(hse) usart_send("Ready @ HSE");
    else usart_send("Ready @ HSI");
    usart_send(", CFGR="); usart_send(u2hexstr(RCC->CFGR));
    usart_send(", CR="); usart_send(u2hexstr(RCC->CR));
    usart_send(", SysFreq="); usart_send(u2str(SysFreq)); usart_putchar('\n');
    uint32_t ctr = Tms;
    while(1){
        if(Tms - ctr > 499){
            ctr = Tms;
            pin_toggle(GPIOB, 1 << 1 | 1 << 0); // toggle LED @ PB0
        }
        if(bufovr){
            bufovr = 0;
            usart_send("bufovr\n");
        }
        char *txt = NULL;
        if(usart_getline(&txt)){
            const char *ans = parse_cmd(txt);
            if(ans) usart_send(ans);
        }
        USB_proc();
        int l = USB_receivestr(inbuff, MAXSTRLEN);
        if(l < 0) USB_sendstr("ERROR: USB buffer overflow or string was too long\n");
        else if(l){
            usart_send("Get by USB ");
            usart_send(u2str(l)); usart_send(" bytes:");
            usart_send(inbuff); usart_putchar('\n');
            const char *ans = parse_cmd(inbuff);
            if(ans) USB_sendstr(ans);
        }
        if(starttest){
            USB_sendstr(test);
            if(0 == --starttest){
#define SENDBOTH(x) do{char *_ = x; usart_send(_); USB_sendstr(_);}while(0)
                SENDBOTH("ENDT=");
                SENDBOTH(u2str(Tms));
                usart_putchar('\n'); USB_putbyte('\n');
            }
        }
    }
}
