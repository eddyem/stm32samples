/*
 * This file is part of the bme280 project.
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
#include "proto.h"
#include "spi.h"
#include "strfunc.h"
#include "usb_dev.h"
#include "BMP280.h"

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
    USBPU_OFF();
    USB_setup();
    BMP280_setup(0, 1);
    uint32_t ctr = Tms, Tmeas = 0;
    USBPU_ON();
    while(1){
        if(Tms - ctr > 499){
            ctr = Tms;
            pin_toggle(GPIOB, 1 << 1 | 1 << 0); // toggle LED @ PB0
        }
        int l = USB_receivestr(inbuff, MAXSTRLEN);
        if(l < 0) USB_sendstr("ERROR: USB buffer overflow or string was too long\n");
        else if(l){
            const char *ans = parse_cmd(inbuff);
            if(ans) USB_sendstr(ans);
        }
        BMP280_process();
        if(Tms - Tmeas > 9999){
            BMP280_status s = BMP280_get_status();
            float Temperature, Pressure, Humidity, Dewpoint;
            if(s == BMP280_NOTINIT || s == BMP280_ERR) BMP280_init();
            else if(s == BMP280_RDY && BMP280_getdata(&Temperature, &Pressure, &Humidity)){
                Dewpoint = Tdew(Temperature, Humidity);
                USB_sendstr("Tdeg="); USB_sendstr(float2str(Temperature, 2)); USB_sendstr("\nPpa=");
                USB_sendstr(float2str(Pressure, 1));
                USB_sendstr("\nPmm="); USB_sendstr(float2str(Pressure * 0.00750062f, 1));
                USB_sendstr("\nH="); USB_sendstr(float2str(Humidity, 1));
                USB_sendstr("\nTdew="); USB_sendstr(float2str(Dewpoint, 1));
                newline();
                Tmeas += 10000;
            }
        }
    }
}
