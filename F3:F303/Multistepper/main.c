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

#include "adc.h"
#include "buttons.h"
#include "can.h"
#include "flash.h"
#include "hardware.h"
#include "pdnuart.h"
#include "proto.h"
#include "steppers.h"
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
    hw_setup(); // GPIO, ADC, timers, watchdog etc.
    USBPU_OFF(); // make a reconnection
    flashstorage_init();
    USB_setup();
    CAN_setup(the_conf.CANspeed);
    adc_setup();
    pdnuart_setup();
    init_steppers();
    USBPU_ON();
    uint32_t ctr = 0;
    CAN_message *can_mesg;
    while(1){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - ctr > 499){
            ctr = Tms;
            LED_blink();
        }
        CAN_proc();
        process_steppers();
        if(CAN_get_status() == CAN_FIFO_OVERRUN){
            USB_sendstr("CAN_FIFO_OVERRUN\n");
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
        if(l < 0) USB_sendstr("USB_BUF_OVERFLOW\n");
        else if(l){
#ifdef EBUG
            USB_sendstr("USB GOT:\n");
            USB_sendstr(inbuff);
            USB_sendstr("\n--------\n");
#endif
            const char *ans = cmd_parser(inbuff);
            if(ans) USB_sendstr(ans);
        }
        process_keys();
    }
}
