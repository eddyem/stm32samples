/*
 * This file is part of the as3935 project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "as3935.h"
#include "hardware.h"
#include "commproto.h"
#include "spi.h"
#include "strfunc.h"
#include "usb_dev.h"

uint8_t DISPLCO[SENSORS_AMOUNT] = {0};

volatile uint32_t Tms = 0;
static char inbuff[RBINSZ];

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

int main(){
    uint8_t oldint[2] = {0}; // ==1 for interrupted pins
    uint32_t lastT = 0;
    StartHSE();
    //flashstorage_init();
    hw_setup();
    USBPU_OFF();
    SysTick_Config(72000);
    USB_setup();
#ifndef EBUG
    iwdg_setup();
#endif
    //usart_setup();
    USBPU_ON();
    // wake-up all sensors and run auto-calibration
    for(int ch = 0; ch < SENSORS_AMOUNT; ++ch){
        CS(ch);
        as3935_wakeup();
        CS_OFF();
    }
    while(1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
        }
        int l = USB_receivestr(inbuff, RBINSZ);
        if(l < 0) USB_sendstr("ERROR: CMD USB buffer overflow or string was too long");
        else if(l){
            const char *ans = parse_cmd(USB_sendstr, inbuff);
            if(ans) USB_sendstr(ans);
        }
        for(int ch = 0; ch < SENSORS_AMOUNT; ++ch){
            if(DISPLCO[ch] == DISPLCO_NOTHING) continue; // don't check IRQ in if it used as clock output
            if(CHK_INT(ch)){
                if(oldint[ch] == 0){
                    oldint[ch] = 1;
                    USB_sendstr("INTERRUPT @ "); USB_putbyte('0'+ch); newline();
                    uint8_t code;
                    CS(ch);
                    int result = as3935_intcode(&code);
                    CS_OFF();
                    if(!result) USB_sendstr("Can't get code\n");
                    else{
                        USB_sendstr("Code: ");
                        switch(code){
                        case INT_NH: USB_sendstr("Noice too high\n"); break;
                        case INT_D: USB_sendstr("Disturber detected\n"); break;
                        case INT_L: USB_sendstr("Lightning interrupt\n"); break;
                        default: USB_sendstr("Unknown\n"); break;
                        }
                    }
                }
            }else oldint[ch] = 0;
        }
    }
    return 0;
}
