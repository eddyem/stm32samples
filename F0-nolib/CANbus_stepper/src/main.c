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
#include "adc.h"
#include "can.h"
#include "can_process.h"
#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "usart.h"
#include "usb.h"
#include "usb_lib.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

/*
void linecoding_handler(usb_LineCoding *lc){
    memcpy(&new_lc, lc, sizeof(usb_LineCoding));
}
void clstate_handler(uint16_t val){
    SEND("change control line state to ");
    printu(val);
    if(val & CONTROL_DTR) SEND(" (DTR)");
    if(val & CONTROL_RTS) SEND(" (RTS)");
    usart_putchar('\n');
}*/

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
    if(curptr[x-1] == '\n'){
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
    uint32_t lastT = 0;
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup(); // + read board address
    usart_setup();
    adc_setup();
    flashstorage_init();
    /*
    if(RCC->CSR & RCC_CSR_IWDGRSTF){ // watchdog reset occured
        usart_send_blocking("WDGRESET=1\n", 11);
    }
    if(RCC->CSR & RCC_CSR_SFTRSTF){ // software reset occured
        usart_send_blocking("SOFTRESET=1\n", 12);
    }
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags
    */
    CAN_setup(the_conf.CANspeed);
    USB_setup();
    iwdg_setup();

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(Tms - lastT > 499){
            lastT = Tms;
        }
        can_proc();
        usb_proc();
        usart_proc(); // switch RS-485 to Rx after last byte sent
        char *txt;
        if((txt = get_USB())){
            cmd_parser(txt, TARGET_USB);
        }
        IWDG->KR = IWDG_REFRESH;
        if(usartrx()){ // usart1 received data, store in in buffer
            usart_getline(&txt);
            cmd_parser(txt, TARGET_USART);
        }
        IWDG->KR = IWDG_REFRESH;
        can_messages_proc();
    }
    return 0;
}

