/*
 * This file is part of the canbus4bta project.
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

#include "adc.h"
#include "proto.h"
#include "strfunc.h"
#include "usart.h"
#include "usb.h"
#include "version.inc"

const char* helpmsg =
    "https://github.com/eddyem/stm32samples/tree/master/F3:F303/CANbus4BTA build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
    "'a' - print ADC values\n"
    "'i' - print USB->ISTR state\n"
    "'p' - toggle USB pullup\n"
    "'N' - read number (dec, 0xhex, 0oct, bbin) and show it in decimal\n"
    "'R' - software reset\n"
    "'T' - get MCU T\n"
    "'U' - get USB status\n"
    "'W' - test watchdog\n"
;

static char stbuf[256], *bptr = NULL;
static int blen = 0;
static void initbuf(){bptr = stbuf; blen = 255; *bptr = 0;}
static void bufputchar(char c){
    if(blen == 0) return;
    *bptr++ = c; --blen;
    *bptr = 0;
}
static void add2buf(const char *s){
    while(blen && *s){
        *bptr++ = *s++;
        --blen;
    }
    *bptr = 0;
}

extern uint8_t usbON;
const char *parse_cmd(const char *buf){
    initbuf();
    if(buf[1] == '\n' || !buf[1]){ // one symbol commands
        switch(*buf){
            case 'a':
                for(int i = 0; i < ADC_TSENS; ++i){
                    bufputchar('A'); bufputchar(i+'0'); bufputchar('=');
                    add2buf(u2str(getADCval(i))); bufputchar('\n');
                }
                return stbuf;
            break;
            case 'i':
                add2buf("USB->ISTR=");
                add2buf(uhex2str(USB->ISTR));
                add2buf(", USB->CNTR=");
                add2buf(uhex2str(USB->CNTR));
            break;
            case 'p':
                pin_toggle(USBPU_port, USBPU_pin);
                add2buf("USB pullup is ");
                if(pin_read(USBPU_port, USBPU_pin)) add2buf("off");
                else add2buf("on");
            break;
            case 'R':
                USB_sendstr("Soft reset\n");
                NVIC_SystemReset();
            break;
            case 'T':
                add2buf("T=");
                add2buf(float2str(getMCUtemp(), 1));
            break;
            case 'U':
                add2buf("USB status: ");
                if(usbON) add2buf("ON");
                else add2buf("OFF");
            break;
            case 'W':
                USB_sendstr("Wait for reboot\n");
                usart_send("Wait for reboot\n");
                while(1){nop();};
            break;
            default:
                return helpmsg;
        }
        bufputchar('\n');
        return stbuf;
    }
    uint32_t Num = 0;
    const char *nxt;
    switch(*buf){ // long messages
        case 'N':
            ++buf;
            nxt = getnum(buf, &Num);
            if(buf == nxt){
                if(Num == 0) return "Wrong number\n";
                return "Integer32 overflow\n";
            }
            add2buf("You give: ");
            add2buf(u2str(Num));
            if(*nxt && *nxt != '\n'){
                add2buf(", the rest of string: ");
                add2buf(nxt);
            }else add2buf("\n");
        break;
        default:
            return buf;
    }
    return stbuf;
}
