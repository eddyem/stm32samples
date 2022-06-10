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
#include "flash.h"
#include "hardware.h"
#include "steppers.h"
#include "strfunct.h"
#include "usb.h"
#include "usb_lib.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

#define USBBUF 63
// usb getline
static char *get_USB(){
    static char tmpbuf[USBBUF+1], *curptr = tmpbuf;
    static int rest = USBBUF;
    uint8_t x = USB_receive((uint8_t*)curptr);
    if(!x) return NULL;
    curptr[x] = 0;
    if(x == 1 && *curptr == 0x7f){ // backspace
        if(curptr > tmpbuf){
            --curptr;
            USND("\b \b");
        }
        return NULL;
    }
    USB_sendstr(curptr); // echo
    if(curptr[x-1] == '\n'){ // || curptr[x-1] == '\r'){
        curptr = tmpbuf;
        rest = USBBUF;
        // omit empty lines
        if(tmpbuf[0] == '\n') return NULL;
        // and wrong empty lines
        if(tmpbuf[0] == '\r' && tmpbuf[1] == '\n') return NULL;
        return tmpbuf;
    }
    curptr += x; rest -= x;
    if(rest <= 0){ // buffer overflow
        curptr = tmpbuf;
        rest = USBBUF;
    }
    return NULL;
}

int main(void){
    uint8_t ctr, len;
    CAN_message *can_mesg;
    uint32_t oS = 0;
    char *txt;
    sysreset();
    SysTick_Config(6000, 1);
    flashstorage_init(); // should be called before any other functions
    gpio_setup();
    USB_setup();
    CAN_setup(the_conf.CANspeed);
    adc_setup();
    init_steppers();
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags
    iwdg_setup();

    while(1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(Tms - oS > 99){ // refresh USB buffer each 100ms
            oS = Tms;
            sendbuf();
        }
        process_keys();
        custom_buttons_process();
        IWDG->KR = IWDG_REFRESH;
        can_proc();
        if(CAN_get_status() == CAN_FIFO_OVERRUN){
            SEND("CAN bus fifo overrun occured!\n");
        }
        IWDG->KR = IWDG_REFRESH;
        usb_proc();
        process_steppers();
        IWDG->KR = IWDG_REFRESH;
        while((can_mesg = CAN_messagebuf_pop())){
            if(can_mesg && ShowMsgs && isgood(can_mesg->ID)){
                IWDG->KR = IWDG_REFRESH;
                len = can_mesg->length;
                printu(Tms);
                SEND(" #");
                printuhex(can_mesg->ID);
                for(ctr = 0; ctr < len; ++ctr){
                    SEND(" ");
                    printuhex(can_mesg->data[ctr]);
                }
                newline();
            }
        }
        IWDG->KR = IWDG_REFRESH;
        if((txt = get_USB())){
            IWDG->KR = IWDG_REFRESH;
            cmd_parser(txt);
        }
    }
    return 0;
}

