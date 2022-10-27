/*
 * This file is part of the pl2303 project.
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

#include "hardware.h"
#include "proto.h"
#include "ringbuffer.h"
#include "usb.h"

#define MAXSTRLEN    128
static char inbuff[MAXSTRLEN];

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    sysreset();
    SysTick_Config(6000, 1);
    hw_setup();
    USB_setup();

    uint32_t ctr = Tms, msgctr = Tms;
    while(1){
        if(Tms - ctr > 499){
            ctr = Tms;
            LED_blink(LED0);
        }
        if(Tms - msgctr > 3999){
            msgctr = Tms;
            USB_sendstr("Test ping message\n");
        }
        USB_proc();
        int len = USB_receivestr(inbuff, MAXSTRLEN);
        if(len < 0) USB_sendstr("ERROR: USB buffer overflow or string was too long\n");
        else if(len){
            const char *ans = parse_cmd(inbuff);
            if(ans) USB_sendstr(ans);
        }
    }
}
