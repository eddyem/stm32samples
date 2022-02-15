/*
 * This file is part of the pl2303 project.
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

#include "hardware.h"
#include "proto.h"
#include "usart.h"
#include "usb.h"

#define USBBUFSZ    127

const char *test = "123456789A123456789B123456789C123456789D123456789E123456789F123456789G123456789H123456789I123456789J123456789K123456789L123456789M123456789N123456789O123456789P123456789Q123456789R123456789S123456789T123456789U123456789V123456789W123456789X123456789Y\n"
        "123456789A123456789B123456789C123456789D123456789E123456789F123456789G123456789H123456789I123456789J123456789K123456789L123456789M123456789N123456789O123456789P123456789Q123456789R123456789S123456789T123456789U123456789V123456789W123456789X123456789Y\n";

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

// usb getline
char *get_USB(){
    static char tmpbuf[USBBUFSZ+1], *curptr = tmpbuf;
    static int rest = USBBUFSZ;
    uint8_t x = USB_receive(curptr);
    if(!x) return NULL;
    curptr[x] = 0;
    if(curptr[x-1] == '\n'){
        curptr = tmpbuf;
        rest = USBBUFSZ;
        return tmpbuf;
    }
    curptr += x; rest -= x;
    if(rest <= 0){ // buffer overflow
        curptr = tmpbuf;
        rest = USBBUFSZ;
        USB_send("USB buffer overflow\n");
    }
    return NULL;
}

int main(void){
    int hse = 0;
    if(StartHSE()){
        hse = 1;
        SysTick_Config((uint32_t)72000); // 1ms
    }else{
        StartHSI();
        SysTick_Config((uint32_t)48000); // 1ms
    }
    hw_setup();
    usart_setup();
    USB_setup();
    if(hse) usart_send("Ready @ HSE");
    else usart_send("Ready @ HSI");
    usart_send(", CFGR="); usart_send(u2hexstr(RCC->CFGR));
    usart_send(", CR="); usart_send(u2hexstr(RCC->CR));
    usart_send(", SysFreq="); usart_send(u2str(SysFreq)); usart_putchar('\n');
    uint32_t ctr = Tms;
    while(1){
        if(Tms - ctr > 499){
            ctr = Tms;
            pin_toggle(GPIOB, 1 << 1 | 1 << 0); // toggle LED @ PB0
        }
        if(bufovr){
            bufovr = 0;
            usart_send("bufovr\n");
        }
        char *txt = NULL;
        if(usart_getline(&txt)){
            const char *ans = parse_cmd(txt);
            if(ans) usart_send(ans);
        }
        usb_proc();
        if((txt = get_USB())){
            usart_send("Get by USB: ");
            usart_send(txt); usart_putchar('\n');
            const char *ans = parse_cmd(txt);
            if(ans) USB_send(ans);
        }
        if(Tlast){
            usart_send("Tlast="); usart_send(u2str(Tlast)); usart_putchar('\n');
            Tlast = 0;
        }
        if(starttest){
            --starttest;
            USB_send(test);
        }
    }
}
