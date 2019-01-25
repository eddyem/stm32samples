/*
 * main.c
 *
 * Copyright 2018 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#include <string.h> // memcpy
#include "stm32f0.h"
#include "hardware.h"
#include "usart.h"
#include "adc.h"
#include "protocol.h"
#include "mainloop.h"

volatile uint32_t Tms = 0;
volatile uint16_t flow_rate = 0; // flow sensor rate
volatile uint16_t flow_cntr = 0; // flow sensor trigger counter
// this variable is global as user need to clear it in protocol.c
uint8_t crit_error = 0; // got critical error, need user acknowledgement

// Called when systick fires
void sys_tick_handler(void){
    ++Tms;
}

static void print_state(uint8_t state){
    if(state == ST_OK){
        put_string("OK\n");
        return;
    }
    if(state & ST_CRITICAL) put_string("CRIT"); // add prefix "CRIT" for critical states
    if(!(state & ST_OK)){ // something changed
        if(state & ST_OFF) put_string("OFF");
        else{
            if(state & ST_FASTER) put_string("FASTER");
            else put_string("SLOWER");
        }
    }
    put_char('\n');
}

int main(void){
    uint32_t lastTflow = 0; // last flow measurement time
    chiller_state ost = {ST_OK, ST_OK, ST_OK, ST_OK}, *st; // old & current chiller states
    char *txt;
    hw_setup();
    SysTick_Config(6000, 1);
    SEND_BLK("Chiller controller v0.1\n");
    if(RCC->CSR & RCC_CSR_IWDGRSTF){ // watchdog reset occured
        SEND_BLK("WDGRESET=1");
    }
    if(RCC->CSR & RCC_CSR_SFTRSTF){ // software reset occured
        SEND_BLK("SOFTRESET=1");
    }
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags
    while (1){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - lastTflow > FLOW_RATE_MS){ // onse per one second check flow sensor counter
            lastTflow = Tms;
            flow_rate = flow_cntr;
            flow_cntr = 0;
            if(crit_error) SEND("CRITICAL=1\n");
        }
        if(usart1_getline(&txt)){ // usart1 received command, process it
            txt = process_command(txt);
        }else txt = NULL;
        if(txt){ // text waits for sending
            while(ALL_OK != usart1_send(txt, 0)){
                IWDG->KR = IWDG_REFRESH;
            }
        }
        IWDG->KR = IWDG_REFRESH;
        //usart1_sendbuf();
        st = mainloop();
        // process state values
        if(st->common_state != ST_OK){
            if(st->common_state & ST_CRITICAL) crit_error = 1;
            put_string("STATE=");
            print_state(st->common_state);
        }
        // other states
        if(st->pump_state != ST_OK){
            put_string("PUMP=");
            print_state(st->pump_state);
        }
        if(st->cooler_state != ST_OK){
            put_string("COOLER=");
            print_state(st->cooler_state);
        }
        if(st->heater_state != ST_OK){
            put_string("HEATER=");
            print_state(st->heater_state);
        }
        memcpy(&ost, st, sizeof(chiller_state));
        usart1_sendbuf();
    }
}
