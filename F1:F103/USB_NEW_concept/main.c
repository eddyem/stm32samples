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
#include "usb_dev.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

#define STRLEN  (256)

int main(void){
    char inbuff[RBINSZ+1];
    uint32_t lastT = 0;
    StartHSE();
    hw_setup();
    USBPU_OFF();
    SysTick_Config(72000);
    USB_setup();
#ifndef EBUG
    iwdg_setup();
#endif
    USBPU_ON();
    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(lastT > Tms || Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
        }
        int l = USB_receivestr(inbuff, RBINSZ);
        if(l < 0) USB_sendstr("ERROR: USB buffer overflow or string was too long\n");
        else if(l){
            USB_sendstr("RECEIVED: _"); USB_sendstr(inbuff); USB_sendstr("_\n");
        }
    }
    return 0;
}
