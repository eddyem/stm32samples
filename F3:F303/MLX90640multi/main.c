/*
 * This file is part of the mlx90640 project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "i2c.h"
#include "mlxproc.h"
#include "proto.h"
#include "strfunc.h"
#include "usb_dev.h"

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
    USBPU_OFF();
    hw_setup();
    i2c_setup(I2C_SPEED_100K);
    USB_setup();
    USBPU_ON();
    uint32_t ctr = Tms, Tlastima[N_SESORS] = {0};
    mlx_continue(); // init state machine
    while(1){
        if(Tms - ctr > 499){
            ctr = Tms;
            pin_toggle(GPIOB, 1 << 1 | 1 << 0); // toggle LED @ PB0
        }
        int l = USB_receivestr(inbuff, MAXSTRLEN);
        if(l < 0) USB_sendstr("USBOVERFLOW\n");
        else if(l){
            const char *ans = parse_cmd(inbuff);
            if(ans) USB_sendstr(ans);
        }
        if(i2c_scanmode){
            uint8_t addr;
            int ok = i2c_scan_next_addr(&addr);
            if(addr == I2C_ADDREND) USND("SCANEND");
            else if(ok){
                U("FOUNDID=");
                USND(uhex2str(addr));
            }
        }
        mlx_process();
        if(cartoon) for(int i = 0; i < N_SESORS; ++i){
            uint32_t Tnow = mlx_lastimT(i);
            if(Tnow != Tlastima[i]){
                fp_t *im = mlx_getimage(i);
                if(im){
                    U("Timage="); USND(u2str(Tnow)); drawIma(im);
                    Tlastima[i] = Tnow;
                }
            }
        }
    }
}
