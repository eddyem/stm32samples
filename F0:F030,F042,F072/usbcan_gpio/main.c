/*
 * This file is part of the usbcangpio project.
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
#include "flash.h"
#include "gpioproto.h"
#include "hardware.h"
#include "canproto.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    sysreset();
    SysTick_Config(6000, 1);
    flashstorage_init();
    gpio_setup();
    USB_setup();
    CAN_setup(the_conf.CANspeed);
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags
#ifndef EBUG
    iwdg_setup();
#endif
    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        CANUSB_process();
        GPIO_process();
    }
    return 0;
}

