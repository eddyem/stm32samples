/*
 * main.c
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include "stm32f1.h"
#include "usart.h"

static volatile uint32_t Tms = 0; // milliseconds from last reset

// Called when systick fires
void sys_tick_handler(void){
    ++Tms;
}

// setup
static void gpio_setup(void){
    // Enable clocks to the GPIO subsystems 
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;
    // Set leds (PB8/PB9) as opendrain output
    GPIOB->CRH = CRH(8, CNF_ODOUTPUT|MODE_SLOW) | CRH(9, CNF_ODOUTPUT|MODE_SLOW);
    // Set buttons (PC0/1) as inputs with weak pullups 
    GPIOC->ODR = 3; // pullups for PC0/1
    GPIOC->CRL = CRL(0, CNF_PUDINPUT|MODE_INPUT) | CRL(1, CNF_PUDINPUT|MODE_INPUT);
}

int main(){
    int L;
    char *txt;
    sysreset();
    StartHSE();
    gpio_setup();
    usart_setup();
    uint32_t Told = 0;
    SysTick_Config(72000); // interrupt each ms
    SEND("Greetings!"); newline();
    pin_set(GPIOB, 3<<8); // turn off LEDs
    /* Do nothing in main loop */
    while (1){
        if(Tms - Told > 499){ // blink LED1 every 1 second
            pin_toggle(GPIOB, 1<<9);
            Told = Tms;
            transmit_tbuf();
        }
        if(usartrx()){ // usart1 received data, store in in buffer
            L = usart_getline(&txt);
            char _1st = txt[0];
            if(L == 2 && txt[1] == '\n'){
                L = 0;
                switch(_1st){
                    case '0': // turn off LED2
                        SEND("LED2 off");
                        pin_set(GPIOB, 1<<8);
                        newline();
                    break;
                    case '1': // turn it on
                        SEND("LED2 on");
                        pin_clear(GPIOB, 1<<8);
                        newline();
                    break;
                    case 's':
                    case 'S':
                        SEND("LED2 state: ");
                        if(pin_read(GPIOB, 1<<8)) SEND("off");
                        else SEND("on");
                        newline();
                    break;
                    default: // help
                        SEND(
                        "'0' - turn off LED2\n"
                        "'1' - turn on LED2\n"
                        "'s' - test LED2 state\n"
                        );
                    break;
                }
            }
            transmit_tbuf();
        }
        if(L){ // echo all other data
            txt[L] = 0;
            usart_send(txt);
            transmit_tbuf();
            L = 0;
        }

        /*
        uint8_t 
        if(pin_read(GPIOC, 1) && pin_read(GPIOC, 2)){ // no buttons present - morze @ LED1 (PB9)
            if(oldctr != blink_ctr){
                uint32_t T = blink_ctr % 18;
                systick_setup(L[T]);
                if(T & 1) pin_set(GPIOB, 1<<9);
                else pin_clear(GPIOB, 1<<9);
                oldctr = blink_ctr;
            }
        }else{ // button pressed: turn ON given LED
            if(pin_read(GPIOC, 1) == 0){ // PC0 pressed (button S2)
                systick_setup(5);
                if(blink_ctr - oldctr > 499){
                    pin_toggle(GPIOB, 1<<9);
                    oldctr = blink_ctr;
                }
            }else pin_set(GPIOB, 1<<9);
            if(pin_read(GPIOC, 2) == 0){ // PC1 pressed (button S3)
                systick_setup(1);
                if(blink_ctr - oldctr > 499){
                    pin_toggle(GPIOB, 1<<8);
                    oldctr = blink_ctr;
                }
            }else pin_set(GPIOB, 1<<8);

        }*/
    }
}

