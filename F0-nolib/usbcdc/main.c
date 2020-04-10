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

#include "can.h"
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
// usb getline
static char *get_USB(){
    static char tmpbuf[512], *curptr = tmpbuf;
    static int rest = 511;
    uint8_t x = USB_receive((uint8_t*)curptr);
    curptr[x] = 0;
    if(!x) return NULL;
    if(curptr[x-1] == '\n'){
        curptr = tmpbuf;
        rest = 511;
        return tmpbuf;
    }
    curptr += x; rest -= x;
    if(rest <= 0){ // buffer overflow
        curptr = tmpbuf;
        rest = 511;
    }
    return NULL;
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
        usart_send("\nUSB buffer overflow!\n"); transmit_tbuf();
        curptr = tmpbuf;
        rest = USBBUF;
    }
    return NULL;
}



int main(void){
    uint32_t lastT = 0;
    uint8_t ctr, len;
    CAN_message *can_mesg;
    char *txt;
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup();
    usart_setup();
    readCANID();
    CAN_setup();

    switchbuff(0);
    SEND("Greetings! My address is ");
    printuhex(getCANID());
    NL();

    if(RCC->CSR & RCC_CSR_IWDGRSTF){ // watchdog reset occured
        SEND("WDGRESET=1\n"); NL();
    }
    if(RCC->CSR & RCC_CSR_SFTRSTF){ // software reset occured
        SEND("SOFTRESET=1\n"); NL();
    }
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags
    iwdg_setup();
    USB_setup();

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(lastT > Tms || Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
            transmit_tbuf(); // non-blocking transmission of data from UART buffer every 0.5s
        }
        can_proc();
        usb_proc();
        if(CAN_get_status() == CAN_FIFO_OVERRUN){
            SEND("CAN bus fifo overrun occured!\n"); NL();
        }
        can_mesg = CAN_messagebuf_pop();
        if(can_mesg){ // new data in buff
            len = can_mesg->length;
            switchbuff(0);
            SEND("got message, len: "); usart_putchar('0' + len);
            SEND(", data: ");
            for(ctr = 0; ctr < len; ++ctr){
                printuhex(can_mesg->data[ctr]);
                usart_putchar(' ');
            }
            NL();
        }
        if(usartrx()){ // usart1 received data, store in in buffer
            usart_getline(&txt);
            IWDG->KR = IWDG_REFRESH;
            cmd_parser(txt, 0);
        }
        if((txt = get_USB())){
            IWDG->KR = IWDG_REFRESH;
            cmd_parser(txt, 1);
        }
    }
    return 0;
}

