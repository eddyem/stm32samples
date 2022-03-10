/*
 * This file is part of the MAX7219 project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "MAX7219.h"
#include "proto.h"
#include "usb.h"
#include "usb_lib.h"

volatile uint32_t Tms = 0;
uint32_t rollpause = 199;
uint8_t roll = 0; // roll screen

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

// usb getline
static char *get_USB(){
    static char tmpbuf[512], *curptr = tmpbuf;
    static int rest = 511;
    int x = USB_receive(curptr);
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
    uint32_t lastT = 0, lastTroll = 0;
    sysreset();
    StartHSE();
    SysTick_Config(72000);

    USBPU_OFF();
    USB_setup();
    hw_setup();
    MAX7219_setup();
    iwdg_setup();
    USBPU_ON();

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
        }
        if(roll && Tms - lastTroll > rollpause){
            MAX7219_rollscreen();
            MAX7219_refresh();
            lastTroll = Tms;
        }
        MAX7219_process();
        usb_proc();
        char *txt;
        const char *ans;
        if((txt = get_USB())){
            ans = parse_cmd(txt);
            if(ans) USB_send(ans);
        }
    }
    return 0;
}

