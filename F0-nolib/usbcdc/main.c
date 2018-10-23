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
#include "usart.h"
#include "can.h"
#include "usb.h"
#include "usb_lib.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

void iwdg_setup(){
    uint32_t tmout = 16000000;
    /* Enable the peripheral clock RTC */
    /* (1) Enable the LSI (40kHz) */
    /* (2) Wait while it is not ready */
    RCC->CSR |= RCC_CSR_LSION; /* (1) */
    while((RCC->CSR & RCC_CSR_LSIRDY) != RCC_CSR_LSIRDY){if(--tmout == 0) break;} /* (2) */
    /* Configure IWDG */
    /* (1) Activate IWDG (not needed if done in option bytes) */
    /* (2) Enable write access to IWDG registers */
    /* (3) Set prescaler by 64 (1.6ms for each tick) */
    /* (4) Set reload value to have a rollover each 2s */
    /* (5) Check if flags are reset */
    /* (6) Refresh counter */
    IWDG->KR = IWDG_START; /* (1) */
    IWDG->KR = IWDG_WRITE_ACCESS; /* (2) */
    IWDG->PR = IWDG_PR_PR_1; /* (3) */
    IWDG->RLR = 1250; /* (4) */
    tmout = 16000000;
    while(IWDG->SR){if(--tmout == 0) break;} /* (5) */
    IWDG->KR = IWDG_REFRESH; /* (6) */
}

int main(void){
    uint32_t lastT = 0;
    uint8_t ctr, len;
    CAN_message *can_mesg;
    int L;
    char *txt;
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup();
    usart_setup();
    //iwdg_setup();
    readCANID();
    CAN_setup();

    SEND("Greetings! My address is ");
    printuhex(getCANID());
    newline();

    if(RCC->CSR & RCC_CSR_IWDGRSTF){ // watchdog reset occured
        SEND("WDGRESET=1\n");
    }
    if(RCC->CSR & RCC_CSR_SFTRSTF){ // software reset occured
        SEND("SOFTRESET=1\n");
    }
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags

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
            SEND("CAN bus fifo overrun occured!\n");
        }
        can_mesg = CAN_messagebuf_pop();
        if(can_mesg){ // new data in buff
            len = can_mesg->length;
            SEND("got message, len: "); usart_putchar('0' + len);
            SEND(", data: ");
            for(ctr = 0; ctr < len; ++ctr){
                printuhex(can_mesg->data[ctr]);
                usart_putchar(' ');
            }
            newline();
        }
        if(usartrx()){ // usart1 received data, store in in buffer
            L = usart_getline(&txt);
            char _1st = txt[0];
            if(L == 2 && txt[1] == '\n'){
                L = 0;
                switch(_1st){
                    case 'f':
                        transmit_tbuf();
                    break;
                    case 'B':
                        can_send_broadcast();
                    break;
                    case 'C':
                        can_send_dummy();
                    break;
                    case 'G':
                        SEND("Can address: ");
                        printuhex(getCANID());
                        newline();
                    break;
                    case 'R':
                        SEND("Soft reset\n");
                        NVIC_SystemReset();
                    break;
                    case 'S':
                        CAN_reinit();
                        SEND("Can address: ");
                        printuhex(getCANID());
                        newline();
                    break;
                    case 'U':
                        USB_send("Test string for USB\n");
                    break;
                    case 'W':
                        SEND("Wait for reboot\n");
                        while(1){nop();};
                    break;
                    default: // help
                        SEND(
                        "'f' - flush UART buffer\n"
                        "'B' - send broadcast dummy byte\n"
                        "'C' - send dummy byte over CAN\n"
                        "'G' - get CAN address\n"
                        "'R' - software reset\n"
                        "'S' - reinit CAN (with new address)\n"
                        "'U' - send test string over USB\n"
                        "'W' - test watchdog\n"
                        );
                    break;
                }
            }
            transmit_tbuf();
        }
        if(L){ // text waits for sending
            txt[L] = 0;
            usart_send(txt);
            L = 0;
        }
    }
    return 0;
}

