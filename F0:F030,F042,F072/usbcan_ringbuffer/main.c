/*
 * This file is part of the usbcanrb project.
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

#include "can.h"
#include "hardware.h"
#include "proto.h"
#include "usb.h"
#include "usb_lib.h"

#define MAXSTRLEN    128

volatile uint32_t Tms = 0;
static char inbuff[MAXSTRLEN];

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    uint32_t lastT = 0;
    CAN_message *can_mesg;
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup();
    USB_setup();
    CAN_setup(100);
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags
#ifndef EBUG
    iwdg_setup();
#endif

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(lastT && (Tms - lastT > 199)){
            LED_off(LED0);
            lastT = 0;
        }
        can_proc();
        if(CAN_get_status() == CAN_FIFO_OVERRUN){
            USB_sendstr("CAN bus fifo overrun occured!\n");
        }
        while((can_mesg = CAN_messagebuf_pop())){
            IWDG->KR = IWDG_REFRESH;
            if(can_mesg && isgood(can_mesg->ID)){
                LED_on(LED0);
                lastT = Tms;
                if(!lastT) lastT = 1;
                if(ShowMsgs){ // new data in buff
                    IWDG->KR = IWDG_REFRESH;
                    uint8_t len = can_mesg->length;
                    printu(Tms);
                    USB_sendstr(" #");
                    printuhex(can_mesg->ID);
                    for(uint8_t ctr = 0; ctr < len; ++ctr){
                        USB_putbyte(' ');
                        printuhex(can_mesg->data[ctr]);
                    }
                    USB_putbyte('\n');
                }
            }
        }
        int l = USB_receivestr(inbuff, MAXSTRLEN);
        if(l < 0) USB_sendstr("ERROR: USB buffer overflow or string was too long\n");
        else if(l) cmd_parser(inbuff);
    }
    return 0;
}

