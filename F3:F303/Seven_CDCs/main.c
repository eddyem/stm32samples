/*
 * This file is part of the SevenCDCs project.
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

#include "cmdproto.h"
#include "debug.h"
#include "hardware.h"
#include "usart.h"
#include "usb.h"

#define MAXSTRLEN    RBINSZ

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    char inbuff[MAXSTRLEN+1];
    USBPU_OFF();
    if(StartHSE()){
        SysTick_Config((uint32_t)72000); // 1ms
    }else{
        StartHSI();
        SysTick_Config((uint32_t)48000); // 1ms
    }
    hw_setup();
    usarts_setup();
    USB_setup();
    USBPU_ON();

    uint32_t ctr = Tms;
    while(1){
        if(Tms - ctr > 499){
            ctr = Tms;
            pin_toggle(GPIOB, 1 << 1 | 1 << 0); // toggle LED @ PB0
            //DBG("blink\n");
        }
        int l = USB_receivestr(CMD_IDX, inbuff, MAXSTRLEN);
        if(l < 0) USB_sendstr(CMD_IDX, "ERROR: USB buffer overflow or string was too long\n");
        else if(l){
            parse_cmd(inbuff);
        }
        l = USB_receivestr(DBG_IDX, inbuff, MAXSTRLEN);
        if(l) USB_sendstr(DBG_IDX, inbuff); // just echo back all from USB-DBG interface
    }
}
