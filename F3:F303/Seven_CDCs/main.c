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
#include "strfunc.h"
#include "usart.h"
#include "usb.h"

#define MAXSTRLEN    RBINSZ

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

static const char *ebufovr = "ERROR: USB buffer overflow or string was too long\n";
static const char *idxnames[MAX_IDX] = {"CMD", "USART1", "USART2", "USART3", "NOFUNCT", "CAN", "DBG"};

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
            //DBGmesg(u2str(Tms));
            //DBGnl();
        }
        for(int i = 0; i < MAX_IDX; ++i){
            int l = USB_receivestr(i, inbuff, MAXSTRLEN);
            if(l < 0){
                USB_sendstr(DBG_IDX, ebufovr);
                if(i == CMD_IDX) USB_sendstr(CMD_IDX, ebufovr);
                continue;
            }
            if(l == 0) continue;
            USB_sendstr(DBG_IDX, idxnames[i]);
            USB_sendstr(DBG_IDX, "> ");
            USB_sendstr(DBG_IDX, inbuff);
            USB_putbyte(DBG_IDX, '\n');
            USB_sendstr(CMD_IDX, idxnames[i]);
            USB_sendstr(CMD_IDX, "> ");
            USB_sendstr(CMD_IDX, inbuff);
            USB_putbyte(CMD_IDX, '\n');
            switch(i){
                case CMD_IDX:
                    parse_cmd(inbuff);
                break;
                default:
                break;
            }
        }
    }
}
