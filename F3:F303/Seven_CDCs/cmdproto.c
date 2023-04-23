/*
 * This file is part of the SevenCDCs project.
 * Copyright 202 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "cmdproto.h"
#include "debug.h"
#include "strfunc.h"
#include "usb.h"
#include "version.inc"

#define SEND(str)       do{USB_sendstr(CMD_IDX, str);}while(0)
#define SENDN(str)      do{USB_sendstr(CMD_IDX, str); USB_putbyte(CMD_IDX, '\n');}while(0)

extern volatile uint32_t Tms;
extern uint8_t usbON;

const char* helpmsg =
    "https://github.com/eddyem/stm32samples/tree/master/F3:F303/PL2303 build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
    "'i' - print USB->ISTR state\n"
    "'N' - read number (dec, 0xhex, 0oct, bbin) and show it in decimal\n"
    "'R' - software reset\n"
    "'U' - get USB status\n"
    "'W' - test watchdog\n"
;

void parse_cmd(const char *buf){
    if(buf[1] == '\n' || !buf[1]){ // one symbol commands
        switch(*buf){
            case 'i':
                SEND("USB->ISTR=");
                SEND(uhex2str(USB->ISTR));
                SEND(", USB->CNTR=");
                SENDN(uhex2str(USB->CNTR));
            break;
            case 'R':
                SENDN("Soft reset");
                USB_sendall(CMD_IDX);
                NVIC_SystemReset();
            break;
            case 'U':
                SEND("USB status: ");
                if(usbON) SENDN("ON");
                else SENDN("OFF");
            break;
            case 'W':
                SENDN("Wait for reboot");
                USB_sendall(CMD_IDX);
                while(1){nop();};
            break;
            default:
                SEND(helpmsg);
        }
        return;
    }
    uint32_t Num = 0;
    const char *nxt;
    switch(*buf){ // long messages
        case 'N':
            ++buf;
            nxt = getnum(buf, &Num);
            if(buf == nxt){
                if(Num == 0) SENDN("Wrong number");
                SENDN("Integer32 overflow");
            }
            SEND("You give: ");
            SEND(u2str(Num));
            if(*nxt && *nxt != '\n'){
                SEND(", the rest of string: ");
                SEND(nxt);
            }else SEND("\n");
        break;
        default:
            SEND(buf);
            return;
    }
}
