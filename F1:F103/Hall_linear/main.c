/*
 * This file is part of the canonmanage project.
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

#include "adc.h"
#include "can.h"
#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "usb.h"

#define USBBUFSZ 256

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

static int isUSB = FALSE;

int main(void){
    char buf[USBBUFSZ];
    StartHSE();
    SysTick_Config(72000);
    flashstorage_init();
    hw_setup();
    adc_setup();
    USBPU_OFF();
    if(ISUSB()){
        isUSB = TRUE;
        USB_setup();
        USBPU_ON();
    }else CAN_setup();
    //uint32_t t = 0;

    while(1){
        IWDG->KR = IWDG_REFRESH;
        /*if(Tms - t > 999){
            t = Tms;
        }*/
        if(isUSB){
            int g = USB_receivestr(buf, USBBUFSZ);
            if(g < 0) USB_sendstr("USB buffer overflow detected\n");
            else if(g > 0) parse_cmd(buf);
        }else{
            CAN_proc();
        }
    }
}
