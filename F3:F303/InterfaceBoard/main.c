/*
 * This file is part of the multiiface project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "Debug.h"
#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "usart.h"
#include "usb_dev.h"

#define MAXSTRLEN    RBINSZ

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    char inbuff[MAXSTRLEN+1];
    uint8_t oldcdc[InterfacesAmount] = {0};
    if(StartHSE()){
        SysTick_Config((uint32_t)72000); // 1ms
    }else{
        StartHSI();
        SysTick_Config((uint32_t)48000); // 1ms
    }
    flashstorage_init();
    hw_setup();
    USBPU_OFF();
    USB_setup();
    //uint32_t ctr = Tms;
    //usb_LineCoding lc = {9600, 0, 0, 8};
    //for(int i = 0; i < 5; ++i) usart_config(i, &lc); // configure all U[S]ARTs for default data
    USBPU_ON();
    while(1){
        // Put here code working WITOUT USB connected
        if(!usbON) continue;
        usarts_process();
        DBGpri();
        /*for(int i = 0; i < 6; ++i){ // just echo for first time
            if(!CDCready[i]) continue;
            int l = USB_receive(i, (uint8_t*)inbuff, MAXSTRLEN);
            if(l) USB_send(i, (uint8_t*)inbuff, l);
        }*/
        // and here is code what should run when USB connected
        if(Config_mode && CDCready[ICFG]){
            /*if(Tms - ctr > 4999){
                ctr = Tms;
                CFGWR("I'm alive\n");
            }*/
            for(int i = 0; i < ICFG; ++i){
                if(oldcdc[i] != CDCready[i]){
                    CFGWR("Interface "); USB_putbyte(ICFG, '1' + i);
                    USB_putbyte(ICFG, ' ');
                    if(!CDCready[i]) CFGWR("dis");
                    CFGWR("connected\n");
                    oldcdc[i] = CDCready[i];
                }
            }
            int l = USB_receivestr(ICFG, inbuff, MAXSTRLEN);
            if(l < 0) CFGWR("ERROR: USB buffer overflow or string was too long\n");
            else if(l){
                CFGWR("PARSING...\n");
                const char *ans = parse_cmd(inbuff);
                if(ans) CFGWRn(ans);
            }
        }
    }
}
