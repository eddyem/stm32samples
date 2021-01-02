/*
 * This file is part of the ws2815 project.
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

#include "hardware.h"
#include "proto.h"
#include "usb.h"
#include "usb_lib.h"
#include "ws2815.h"
#include "hsv.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

// usb getline
char *get_USB(){
    static char tmpbuf[512], *curptr = tmpbuf;
    static int rest = 511;
    uint8_t x = USB_receive((uint8_t*)curptr);
    curptr[x] = 0;
    if(!x) return NULL;
    if(curptr[x-1] == '\n'){
        curptr = tmpbuf;
        rest = 511;
        return tmpbuf;
    }
    curptr += x; rest -= x;
    if(rest <= 0){ // buffer overflow
        curptr = tmpbuf;
        rest = 511;
    }
    return NULL;
}

static uint16_t Harray[LEDS_NUM]; // Hue

static void rstarray(){
    rstcounter = 0;
    for(uint16_t i = 0; i < LEDS_NUM; ++i){
        Harray[i] = i;
        ws2815setpix(i, hsv2grb(Harray[i], S, V));
    }
    ws2815start();
}

int main(void){
    uint32_t lastT = 0, lastTc = 0;
    sysreset();
    StartHSE();
    hw_setup();
    SysTick_Config(72000);

    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags

    USBPU_OFF();
    USB_setup();
    iwdg_setup();
    USBPU_ON();

    rstarray();
    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
        }
        if(Tms - lastTc > 9){
            lastTc = Tms;
            if(rstcounter){
                rstarray();
            }else if(!pause){
                for(uint16_t i = 0; i < LEDS_NUM; ++i){
                    if(++Harray[i] > 359) Harray[i] = 0;
                    uint16_t H = Harray[i];
                       ws2815setpix(i, hsv2grb(H, S, V));
                }
                ws2815start();
            }
        }
        usb_proc();
        char *txt, *ans;
        if((txt = get_USB())){
            IWDG->KR = IWDG_REFRESH;
            ans = (char*)parse_cmd(txt);
            if(ans){
                uint16_t l = 0; char *p = ans;
                while(*p++) l++;
                USB_send((uint8_t*)ans, l);
            }
        }
    }
    return 0;
}
