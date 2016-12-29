/*
 * main.c
 *
 * Copyright 2014 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include "main.h"
#include "hardware_ini.h"
#include "cdcacm.h"
#include "timer.h"

volatile uint32_t Timer = 0; // global timer (milliseconds)
usbd_device *usbd_dev;
void check_btns();

int main(){
    uint32_t Old_timer = 0;
    // RCC clocking: 8MHz oscillator -> 72MHz system
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
    GPIO_init();
    usb_disconnect(); // turn off USB while initializing all
    // USB
    usbd_dev = USB_init();
    // SysTick is a system timer with 1ms period
    SysTick_init();
    tim2_init(); // start transmission
    usb_connect(); // turn on USB
    while(1){
        usbd_poll(usbd_dev);
        if(usbdatalen){ // there's something in USB buffer
            usbdatalen = parce_incoming_buf(usbdatabuf, usbdatalen);
        }
        check_btns();
        if(Timer - Old_timer > 999){ // one-second cycle
            Old_timer += 1000;
        }else if(Timer < Old_timer){ // Timer overflow
            Old_timer = 0;
        }
    }
}

// check buttons +/-
void check_btns(){
    static uint8_t oldstate[2] = {1,1}, done[2] = {0,0}; // old buttons state, event handler flag
    uint8_t i;
    const char ltr[2] = {'+', '-'};
    const uint32_t pins[2] = {BTN_PLUS_PIN, BTN_MINUS_PIN};
    static uint32_t Old_timer[2] = {0,0};
    for(i = 0; i < 2; i++){
        uint8_t new = gpio_get(BTNS_PORT, pins[i]) ? 1 : 0;
        // whe check button state at 3ms after event; don't mind events for 50ms after first
        uint32_t O = Old_timer[i];
        if(!O){ // no previous pauses
                // button was pressed or released just now or holded
                if(new != oldstate[i] || !new){
                    Old_timer[i] = Timer;
                    oldstate[i] = new;
                }
        }else{ // noice or real event?
            if(Timer - O < 3){ // less than 3ms from last event
                if(new != oldstate[i]){ // noice or button released
                    oldstate[i] = new;
                    Old_timer[i] = 0;
                }
                continue;
            }
            // more than 50ms from last event
            if(Timer - O > 50 || O > Timer){ // clear new events masking
                oldstate[i] = new;
                Old_timer[i] = 0;
                done[i] = 0;
            }else{
                if(!new && !done[i]){ // button pressed: just now or hold
                    usb_send(ltr[i]);
                    newline();
                    if(i) decrease_speed(); // "+"
                    else increase_speed();
                    done[i] = 1;
                }
            }
        }
    }
}


/**
 * SysTick interrupt: increment global time & send data buffer through USB
 */
void sys_tick_handler(){
    Timer++;
    usbd_poll(usbd_dev);
    usb_send_buffer();
}

// pause function, delay in ms
void Delay(uint16_t _U_ time){
    uint32_t waitto = Timer + time;
    while(Timer < waitto);
}

/**
 * print current time in milliseconds: 4 bytes for ovrvlow + 4 bytes for time
 * with ' ' as delimeter
 */
void print_time(){
    print_int(Timer);
}
