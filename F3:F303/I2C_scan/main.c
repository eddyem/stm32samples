/*
 * This file is part of the i2cscan project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "i2c.h"
#include "hardware.h"
#include "proto.h"
#include "strfunc.h"
#include "usb.h"

#define MAXSTRLEN    RBINSZ

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    char inbuff[MAXSTRLEN+1];
    if(StartHSE()){
        SysTick_Config((uint32_t)72000); // 1ms
    }else{
        StartHSI();
        SysTick_Config((uint32_t)48000); // 1ms
    }
    hw_setup();
    i2c_setup(I2C_SPEED_10K); // start from lowest speed
    USB_setup();
    uint32_t ctr = Tms;
    while(1){
        if(Tms - ctr > 499){
            ctr = Tms;
            pin_toggle(GPIOB, 1 << 1 | 1 << 0); // toggle LED @ PB0
        }
        USB_proc();
        int l = USB_receivestr(inbuff, MAXSTRLEN);
        if(l < 0) USND("ERROR: USB buffer overflow or string was too long\n");
        else if(l){
            const char *ans = parse_cmd(inbuff);
            if(ans) USND(ans);
        }
        if(i2c_scanmode){
            uint8_t addr;
            int ok = i2c_scan_next_addr(&addr);
            if(addr == I2C_ADDREND) USND("Scan ends\n");
            else if(ok){
                USND(uhex2str(addr));
                USND(" ("); USND(u2str(addr));
                USND(") - found device\n");
            }
        }
        if(i2c_got_DMA){
            i2c_bufdudump();
            i2c_got_DMA = 0;
        }
    }
}
