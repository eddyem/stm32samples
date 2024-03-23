/*
 * This file is part of the canbus4bta project.
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
#include "encoder.h"
#include "flash.h"
#include "gpio.h"
#include "hardware.h"
#include "textfunctions.h"
#include "usart.h"
#include "usb.h"

#define MAXSTRLEN    RBINSZ

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

TRUE_INLINE void showCANmessages(){
    CAN_message *can_mesg = NULL;
    while((can_mesg = CAN_messagebuf_pop())){
        IWDG->KR = IWDG_REFRESH;
        printu(Tms);
        USB_sendstr(" #");
        printuhex(can_mesg->ID);
        for(int ctr = 0; ctr < can_mesg->length; ++ctr){
            USB_putbyte(' ');
            printuhex(can_mesg->data[ctr]);
        }
        newline();
    }
}

int main(void){
    char inbuff[MAXSTRLEN+1];
    uint32_t encT = 0; // time of ENC and LIM data sent (PEP emulation)
    USBPU_OFF();
    if(StartHSE()){
        SysTick_Config((uint32_t)72000); // 1ms
    }else{
        StartHSI();
        SysTick_Config((uint32_t)48000); // 1ms
    }
    flashstorage_init();
    hw_setup();
    // getSwitches() and set module role & CAN ID
    CAN_setup(the_conf.CANspeed);
    encoder_setup();
    USBPU_ON();
    while(1){
        IWDG->KR = IWDG_REFRESH;
        CAN_proc();
        CAN_status stat = CAN_get_status();
        if(stat == CAN_FIFO_OVERRUN){
            USB_sendstr("CAN bus fifo overrun occured!\n");
            // TODO: buzzer short
        }else if(stat == CAN_ERR){
            // TODO: buzzer errors
            CAN_reinit(0);
        }
        if(cansniffer) showCANmessages();
        int l = USB_receivestr(inbuff, MAXSTRLEN);
        if(l < 0) USB_sendstr("error=usboverflow\n");
        else if(l){
            const char *ans = run_text_cmd(inbuff);
            if(ans) USB_sendstr(ans);
        }
        ESW_process();
        static uint8_t oldswitches = 0;
        uint8_t new = getESW(1);
        if(oldswitches != new){
            oldswitches = new;
            USB_sendstr("ESW changed @"); printu(Tms);
            USB_sendstr(" to "); printuhex(new); newline();
            CANsendLim();
        }
        encoder_process();
        if(FLAG(EMULATE_PEP)){
            if(Tms - encT > ENCODER_PERIOD){
                encT = Tms;
                CANsendEnc();
            }
        }
    }
}
