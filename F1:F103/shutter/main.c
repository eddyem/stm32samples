/*
 * This file is part of the shutter project.
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
#include "shutter.h"
#include "usb.h"

#define MAXSTRLEN    RBINSZ

// eash ERRPERIOD ms show error message if SHTR_FB is low
#define ERRPERIOD   499

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    char inbuff[MAXSTRLEN+1];
    StartHSE();
    SysTick_Config(72000);
    USBPU_OFF();
    flashstorage_init();
    hw_setup();
    USB_setup();
    // close shutter and only after that turn on USB pullup
    while(!close_shutter() && Tms < the_conf.waitingtime) IWDG->KR = IWDG_REFRESH;
    USBPU_ON();

    uint32_t Terr = Tms + 2*ERRPERIOD;
    while(1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(shutterstate == SHUTTER_ERROR && Tms - Terr > ERRPERIOD){
            USB_sendstr("shutter=error\n");
            Terr = Tms;
        }
        int l = USB_receivestr(inbuff, MAXSTRLEN);
        if(l < 0) USB_sendstr("usb=error\n");
        else if(l){
            const char *ans = parse_cmd(inbuff);
            if(ans) USB_sendstr(ans);
        }
        process_shutter();
    }
}
