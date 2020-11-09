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
    ++Tms;
}

#define USBBUF 63
// usb getline
static char *get_USB(int idx){
    static char tmpbuf[8][USBBUF+1];
    static char *curptr[8] = {tmpbuf[0], tmpbuf[1], tmpbuf[2], tmpbuf[3], tmpbuf[4], tmpbuf[5], tmpbuf[6], tmpbuf[7]};
    static int rest[8] = {USBBUF, USBBUF, USBBUF, USBBUF, USBBUF, USBBUF, USBBUF, USBBUF};
    uint8_t x = USB_receive(idx, (uint8_t*)curptr[idx]);
    if(!x) return NULL;
    curptr[idx][x] = 0;
    if(x == 1 && *curptr[idx] == 0x7f){ // backspace
        if(curptr[idx] > tmpbuf[idx]){
            --curptr[idx];
            USND(idx, "\b \b");
        }
        return NULL;
    }
    USB_sendstr(idx, curptr[idx]); // echo
    if(curptr[idx][x-1] == '\n'){ // || curptr[x-1] == '\r'){
        curptr[idx] = tmpbuf[idx];
        rest[idx] = USBBUF;
        // omit empty lines
        if(tmpbuf[idx][0] == '\n') return NULL;
        // and wrong empty lines
        if(tmpbuf[idx][0] == '\r' && tmpbuf[idx][1] == '\n') return NULL;
        return tmpbuf[idx];
    }
    curptr[idx] += x; rest[idx] -= x;
    if(rest[idx] <= 0){ // buffer overflow
        curptr[idx] = tmpbuf[idx];
        rest[idx] = USBBUF;
    }
    return NULL;
}

int main(void){
    uint32_t lastT = 0;
    sysreset();
    StartHSE();
    SysTick_Config(72000);
    hw_setup();
    USBPU_OFF();
   /*if(RCC->CSR & RCC_CSR_IWDGRSTF){ // watchdog reset occured
        SEND("WDGRESET=1"); newline();
    }
    if(RCC->CSR & RCC_CSR_SFTRSTF){ // software reset occured
        SEND("SOFTRESET=1"); newline();
    }*/
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags

    USB_setup();
    iwdg_setup();
    USBPU_ON();
    while(1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
        }
        usb_proc();
        for(uint8_t idx = 1; idx < 8; ++idx){
            char *txt = get_USB(idx);
            if(txt){
                IWDG->KR = IWDG_REFRESH;
                cmd_parser(txt);
            }
        }
    }
    return 0;
}

