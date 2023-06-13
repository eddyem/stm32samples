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

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    char inbuff[MAXSTRLEN+1];
    StartHSE();
    hw_setup();
    SysTick_Config(72000);
    USBPU_OFF();
    hw_setup();
    usart_setup();
    USB_setup();
    USBPU_ON();

    uint32_t ctr = Tms;
    while(1){
        if(Tms - ctr > 499){
            //USEND("ping\n");
            ctr = Tms;
            LED_blink(LED0);
            usart_transmit();
        }
        if(usartovr()){
            usart_send("USART buffer overfull\n");
        }
        char *txt = NULL;
        int l = USB_receivestr(inbuff, MAXSTRLEN);
        if(l < 0) USB_sendstr("ERROR: USB buffer overflow or string was too long\n");
        else if(l){
            USEND("Got USB data\n");
            const char *ans = parse_cmd(inbuff);
            if(ans) USB_sendstr(ans);
        }
        if(usartrx()){
            if(usart_getline(&txt)){
                const char *ans = parse_cmd(txt);
                if(ans) USEND(ans);
            }
        }
    }
}
