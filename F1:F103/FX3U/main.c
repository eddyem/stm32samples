/*
 * This file is part of the fx3u project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "can.h"
#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "strfunc.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    uint32_t lastT = 0;
    CAN_message *can_mesg;
    StartHSE();
    SysTick_Config(72000);
    flashstorage_init();
    gpio_setup(); // should be run before other peripherial setup
    adc_setup();
    usart_setup(the_conf.rs232speed);
    CAN_setup(the_conf.canspeed);
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags
#ifndef EBUG
    iwdg_setup();
#endif
    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(Tms - lastT > 499){ // throw out short messages twice per second
            usart_transmit();
            lastT = Tms;
        }
        can_proc();
        CAN_status st = CAN_get_status();
        if(st == CAN_FIFO_OVERRUN){
            usart_send("CAN bus fifo overrun occured!\n");
        }else if(st == CAN_ERR){
            usart_send("Some CAN error occured\n");
        }
        while((can_mesg = CAN_messagebuf_pop())){
            IWDG->KR = IWDG_REFRESH;
            // TODO: check ID and process messages
            if(flags.can_monitor){
                lastT = Tms;
                if(!lastT) lastT = 1;
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
        char *str;
        int g = usart_getline(&str);
        if(g < 0) usart_send("USART IN buffer overflow!\n");
        else if(g > 0) cmd_parser(str);
    }
    return 0;
}

