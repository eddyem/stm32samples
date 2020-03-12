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

int main(void){
    uint32_t lastT = 0;
    char tmpbuf[129];
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup();
    usart_setup();
    adc_setup();
    if(RCC->CSR & RCC_CSR_IWDGRSTF){ // watchdog reset occured
        usart_send_blocking("WDGRESET=1\n", 11);
    }
    if(RCC->CSR & RCC_CSR_SFTRSTF){ // software reset occured
        usart_send_blocking("SOFTRESET=1\n", 12);
    }
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags
    USB_setup();
    //iwdg_setup();

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(lastT > Tms || Tms - lastT > 499){
            lastT = Tms;
        }
        usb_proc();
        usart_proc(); // switch RS-485 to Rx after last byte sent
        uint8_t r = 0;
        if((r = USB_receive(tmpbuf, 128))){
            tmpbuf[r] = 0;
            cmd_parser(tmpbuf, TARGET_USB);
        }
        if(usartrx()){ // usart1 received data, store in in buffer
            char *txt = NULL;
            r = usart_getline(&txt);
/*
buftgt(TARGET_USB);
addtobuf("got ");
printu(r);
addtobuf(" bytes over USART:\n");
addtobuf(txt);
addtobuf("\n====\n");
sendbuf();
*/
            cmd_parser(txt, TARGET_USART);
        }
    }
    return 0;
}

