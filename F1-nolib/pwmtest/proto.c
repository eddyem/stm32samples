/*
 * This file is part of the pwmtest project.
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

#include "proto.h"
#include "usb.h"

#define USND(str)  do{USB_send(str, sizeof(str)-1);}while(0)
const char *parse_cmd(const char *buf){
    if(buf[1] != '\n') return buf;
    switch(*buf){
        /*case 'p':
            pin_toggle(USBPU_port, USBPU_pin);
            USND("USB pullup is ");
            if(pin_read(USBPU_port, USBPU_pin)) USND("off\n");
            else USND("on\n");
            return NULL;
        break;*/
        case 'R':
            USND("Soft reset\n");
            NVIC_SystemReset();
        break;
        case 'W':
            USND("Wait for reboot\n");
            while(1){nop();};
        break;
        default: // help
            return
            "'R' - software reset\n"
            "'W' - test watchdog\n"
            ;
        break;
    }
    return NULL;
}
