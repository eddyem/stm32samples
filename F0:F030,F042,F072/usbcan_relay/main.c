/*
 * This file is part of the canrelay project.
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

#include "adc.h"
#include "buttons.h"
#include "can.h"
#include "custom_buttons.h"
#include "hardware.h"
#include "proto.h"
#include "usb.h"
#include "usb_lib.h"

#define MAXSTRLEN    128

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    char inbuff[MAXSTRLEN];
    uint8_t ctr, len;
    CAN_message *can_mesg;
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup();
    adc_setup();
    tim1_setup();
    USB_setup();
    CAN_setup(DEFAULT_CAN_SPEED);
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags
    iwdg_setup();

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        process_keys();
        custom_buttons_process();
        can_proc();
        if(CAN_get_status() == CAN_FIFO_OVERRUN){
            USND("CAN bus fifo overrun occured!");
        }
        while((can_mesg = CAN_messagebuf_pop())){
            if(can_mesg && isgood(can_mesg->ID)){
                if(ShowMsgs){ // new data in buff
                    IWDG->KR = IWDG_REFRESH;
                    len = can_mesg->length;
                    printu(Tms);
                    USB_sendstr(" #");
                    printuhex(can_mesg->ID);
                    for(ctr = 0; ctr < len; ++ctr){
                        USB_putbyte(' ');
                        printuhex(can_mesg->data[ctr]);
                    }
                    newline();
                }
            }
        }
        int l = USB_receivestr(inbuff, MAXSTRLEN);
        IWDG->KR = IWDG_REFRESH;
        if(l < 0) USB_sendstr("ERROR: USB buffer overflow or string was too long\n");
        else if(l) cmd_parser(inbuff);
    }
    return 0;
}

