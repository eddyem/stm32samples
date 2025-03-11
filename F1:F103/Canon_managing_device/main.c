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

#include "can.h"
#include "canon.h"
#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "spi.h"
#include "usb_dev.h"

#define USBBUFSZ    127

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    char inbuff[256];
    sysreset();
    StartHSE();
    SysTick_Config(72000);
    flashstorage_init();
    hw_setup();
    if(ISUSB()){
        USBPU_OFF();
        USB_setup();
        USBPU_ON();
    }else CAN_setup(the_conf.canspeed, the_conf.canID);
    spi_setup();

    uint32_t SPIctr = Tms;
    while(1){
        IWDG->KR = IWDG_REFRESH;
        int l = USB_receivestr(inbuff, 255);
        if(l > 0){
            parse_cmd(inbuff); // call it even for NULL (if `flood` is running)
            if(Tms - SPIctr > CANONPROC_INTERVAL){ // not more than once per 10ms
                SPIctr = Tms;
                canon_proc();
            }
        }
    }
}
