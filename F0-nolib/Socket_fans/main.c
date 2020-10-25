/*
 * main.c
 *
 * Copyright 2017 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "hardware.h"
#include "proto.h"
#include "usb.h"
#include "usb_lib.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    static uint32_t actr = 0;
    ++Tms;
    if(++actr == 1000){ // RPM counter
        Cooler0speed = TIM3->CNT/2;
        TIM3->CNT = 0;
        Cooler1speed = Cooler1RPM/2;
        Cooler1RPM = 0;
        actr = 0;
    }
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
    //uint32_t lastT = 0;
    char *txt;
    sysreset();
    SysTick_Config(6000, 1);
    HW_setup();
    USB_setup();
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags
    iwdg_setup();

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        /*if(lastT && (Tms - lastT > 199)){
            LED_off(LED0);
            lastT = 0;
        }*/
        usb_proc();
        if((txt = get_USB())){
            IWDG->KR = IWDG_REFRESH;
            cmd_parser(txt);
        }
    }
    return 0;
}
