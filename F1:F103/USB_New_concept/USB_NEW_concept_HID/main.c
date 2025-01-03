/*
 * This file is part of the I2Cscan project.
 * Copyright 2020 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "hardware.h"
#include "keycodes.h"
#ifdef EBUG
#include "usart.h"
#endif
#include "usb_dev.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

/*
 * buf[0]: 1 - report ID
 * buf[1]: bit2 - middle button, bit1 - right, bit0 - left
 * buf[2]: move X
 * buf[3]: move Y
 * buf[4]: wheel
 */
static void move_mouse(int8_t x, int8_t y){
    int8_t buf[5] = {1,0,0,0,0};
    buf[2] = x; buf[3] = y;
    USB_send((uint8_t*)buf, 5);
}

/*
 * Keyboard buffer:
 * buf[1]: MOD
 * buf[2]: reserved
 * buf[3]..buf[8] - keycodes 1..6
 */
static void send_word(const char *wrd){
    char last = 0;
    do{
        // release key before pressing it again
        if(last == *wrd) USB_send(release_key(), USB_KEYBOARD_REPORT_SIZE);
        last = *wrd;
        USB_send(press_key(last), USB_KEYBOARD_REPORT_SIZE);
        IWDG->KR = IWDG_REFRESH;
    }while(*(++wrd));
    USB_send(release_key(), USB_KEYBOARD_REPORT_SIZE);
}

int main(void){
    uint32_t lastT = 0, lastsend = 0;
    StartHSE();
    hw_setup();
    USBPU_OFF();
    SysTick_Config(72000);
#ifdef EBUG
    usart_setup();
    uint32_t tt = 0;
#endif
    USB_setup();
#ifndef EBUG
    iwdg_setup();
#endif
    USBPU_ON();
    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
        }
#ifdef EBUG
        if(Tms != tt){
            __disable_irq();
            usart_transmit();
            tt = Tms;
            __enable_irq();
        }
#endif
        if(Tms - lastsend > 9999){
            if(usbON){
                send_word("Hello ");
                move_mouse(100, 100);
                //move_mouse(0,0);
                lastsend = Tms;
            }
        }
    }
    return 0;
}
