/*
 * This file is part of the multistepper project.
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

#include "can.h"
#include "hardware.h"
#include "proto.h"
#include "usb.h"

#define MAXSTRLEN    RBINSZ

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    char inbuff[MAXSTRLEN+1];
    if(StartHSE()){
        SysTick_Config((uint32_t)72000); // 1ms
    }else{
        StartHSI();
        SysTick_Config((uint32_t)48000); // 1ms
    }
    hw_setup();
    // here we should check the debug mode and turn off SWD if no
    if(BTN_state(0) && BTN_state(1)){ // NO debug - turn ON SWDIO (AF0)
        GPIOA->MODER = (GPIOA->MODER & ~MODER_CLR(13)) | MODER_AF(13);
    }else{
        USB_setup();
        USBPU_ON();
    }
    CAN_setup(100);
    uint32_t ctr = 0;
    CAN_message *can_mesg;
    while(1){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - ctr > 499){
            ctr = Tms;
        }
        can_proc();
        USB_proc();
        if(CAN_get_status() == CAN_FIFO_OVERRUN){
            USB_sendstr("CAN bus fifo overrun occured!\n");
        }
        while((can_mesg = CAN_messagebuf_pop())){
            if(can_mesg && isgood(can_mesg->ID)){
                if(ShowMsgs){ // display message content
                    IWDG->KR = IWDG_REFRESH;
                    uint8_t len = can_mesg->length;
                    printu(Tms);
                    USB_sendstr(" #");
                    printuhex(can_mesg->ID);
                    for(uint8_t i = 0; i < len; ++i){
                        USB_putbyte(' ');
                        printuhex(can_mesg->data[i]);
                    }
                    USB_putbyte('\n');
                }
            }
        }
        int l = USB_receivestr(inbuff, MAXSTRLEN);
        if(l < 0) USB_sendstr("ERROR: USB buffer overflow or string was too long\n");
        else if(l) cmd_parser(inbuff);
    }
}
