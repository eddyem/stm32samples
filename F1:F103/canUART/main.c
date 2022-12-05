/*
 * This file is part of the canuart project.
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

#include "can.h"
#include "hardware.h"
#include "proto.h"
#include "usart.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    uint32_t lastT = 0;
#ifdef BLUEPILL
    uint32_t bplastT = 0;
#endif
    CAN_message *can_mesg;
    StartHSE();
    SysTick_Config(72000);
    gpio_setup();
    usart_setup();
    CAN_setup(100);
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags
#ifndef EBUG
    iwdg_setup();
#endif

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(lastT && (Tms - lastT > 199)){
            LED_off(LED0);
            lastT = 0;
        }
#ifdef BLUEPILL
        if(Tms - bplastT > 499){
            pin_toggle(LEDB_port, LEDB_pin);
            bplastT = Tms;
        }
#endif
        can_proc();
        CAN_status st = CAN_get_status();
        if(st == CAN_FIFO_OVERRUN){
            usart_send("CAN bus fifo overrun occured!\n");
        }else if(st == CAN_ERR){
            usart_send("Some CAN error occured\n");
        }
        while((can_mesg = CAN_messagebuf_pop())){
            if(can_mesg && isgood(can_mesg->ID)){
                LED_on(LED0);
                lastT = Tms;
                if(!lastT) lastT = 1;
                if(ShowMsgs){ // new data in buff
                    IWDG->KR = IWDG_REFRESH;
                    uint8_t len = can_mesg->length;
                    printu(Tms);
                    usart_send(" #");
                    printuhex(can_mesg->ID);
                    for(uint8_t ctr = 0; ctr < len; ++ctr){
                        usart_putchar(' ');
                        printuhex(can_mesg->data[ctr]);
                    }
                    usart_putchar('\n');
                }
            }
        }
        char *str;
        int g = usart_getline(&str);
        if(g < 0) usart_send("USART buffer overflow!\n");
        else if(g > 0) cmd_parser(str);
        usart_transmit();
    }
    return 0;
}

