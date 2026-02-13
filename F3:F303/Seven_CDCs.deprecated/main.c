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

#include "can.h"
#include "canproto.h"
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
static const char *idxnames[MAX_IDX] = {"CMD", "DBG", "USART1", "USART2", "USART3", "NOFUNCT", "CAN"};

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
    CAN_setup(100);
    USBPU_ON();

    CAN_message *can_mesg;
    uint32_t ctr = Tms;
    while(1){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - ctr > 499){
            ctr = Tms;
            pin_toggle(GPIOB, 1 << 1 | 1 << 0); // toggle LED @ PB0
            //USB_sendstr(CMD_IDX, "1");
            //DBGmesg(u2str(Tms));
            //DBGnl();
        }
        can_proc();
        if(CAN_get_status() == CAN_FIFO_OVERRUN){
            USB_sendstr(CAN_IDX, "CAN bus fifo overrun occured!\n");
        }
        while((can_mesg = CAN_messagebuf_pop())){
            if(isgood(can_mesg->ID)){
                if(ShowMsgs){ // display message content
                    IWDG->KR = IWDG_REFRESH;
                    uint8_t len = can_mesg->length;
                    USB_sendstr(CAN_IDX, u2str(Tms));
                    USB_sendstr(CAN_IDX, " #");
                    USB_sendstr(CAN_IDX, uhex2str(can_mesg->ID));
                    for(uint8_t i = 0; i < len; ++i){
                        USB_putbyte(CAN_IDX, ' ');
                        USB_sendstr(CAN_IDX, uhex2str(can_mesg->data[i]));
                    }
                    USB_putbyte(CAN_IDX, '\n');
                }
            }
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
            switch(i){
                case CMD_IDX:
                    parse_cmd(inbuff);
                break;
                case CAN_IDX:
                    cmd_parser(inbuff);
                break;
                default:
                break;
            }
        }
    }
}
