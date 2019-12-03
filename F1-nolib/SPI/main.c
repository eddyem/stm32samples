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
#include "spi.h"
#include "usb.h"
#include "usb_lib.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

char *parse_cmd(char *buf){
    const uint8_t array[] = {0xf0, 0xa5, 0x5a, 0x0f};
    if(buf[1] != '\n') return buf;
    switch(*buf){
        case 'D':
            if(SPI_transmit(array, 4)) USB_send("Error in DMA transmission\n");
            else USB_send("Transmitted\n");
        break;
        case 'S':
            if(SPI1->SR & SPI_SR_TXE){
                *(uint8_t *)&(SPI1->DR) = 0xa5;
                USB_send("sent 0xa5\n");
            }else USB_send("SR_TXE isn't empty\n");
        break;
        default: // help
            return
            "'S' - send 0xa5 directly\n"
            "'D' - send 4 bytes by DMA\n"
            ;
        break;
    }
    return NULL;
}

// usb getline
char *get_USB(){
    static char tmpbuf[512], *curptr = tmpbuf;
    static int rest = 511;
    int x = USB_receive(curptr, rest);
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
}

int main(void){
    uint32_t lastT = 0;
    sysreset();
    StartHSE();
    SysTick_Config(72000);
    hw_setup();
    USB_setup();

    while (1){
        if(lastT > Tms || Tms - lastT > 499){
            pin_toggle(GPIOA, (1<<6));
            LED_blink(LED0);
            lastT = Tms;
        }
        usb_proc();
        char *txt, *ans;
        if((txt = get_USB())){
            ans = parse_cmd(txt);
            if(ans) USB_send(ans);
        }
    }
    return 0;
}

