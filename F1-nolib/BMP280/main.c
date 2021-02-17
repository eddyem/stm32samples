/*
 * This file is part of the BMP280 project.
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

#include "BMP280.h"
#include "dewpoint.h"
#include "hardware.h"
#include "proto.h"
#include "usb.h"
#include "usb_lib.h"

#define USBBUFSZ    127

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

// usb getline
static char *get_USB(){
    static char tmpbuf[USBBUFSZ+1], *curptr = tmpbuf;
    static int rest = USBBUFSZ;
    int x = USB_receive(curptr);
    curptr[x] = 0;
    if(!x) return NULL;
    if(curptr[x-1] == '\n'){
        curptr = tmpbuf;
        rest = USBBUFSZ;
        return tmpbuf;
    }
    curptr += x; rest -= x;
    if(rest <= 0){ // buffer overflow
        curptr = tmpbuf;
        rest = USBBUFSZ;
        USB_send("USB buffer overflow\n");
    }
    return NULL;
}

static void showd(int32_t mT, int32_t mH){
    USB_send("Dewpoint=");
    USB_send(u2str(dewpoint(mT, mH)));
    USB_send("*10degrC\n");
}

int main(void){
    uint32_t lastT = 0;
    sysreset();
    StartHSE();
    hw_setup();
    SysTick_Config(72000);

    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags

    USBPU_OFF();
    USB_setup();
    iwdg_setup();
    USBPU_ON();

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(lastT > Tms || Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
        }
        usb_proc();
        BMP280_process();
        BMP280_status s = BMP280_get_status();
        if(s == BMP280_RDY){ // data ready - get it
            int32_t T;
            uint32_t P, H;
            if(BMP280_getdata(&T, &P, &H)){
                USB_send("T="); USB_send(i2str(T)); USB_send(" (*100degC)\nP=");
                USB_send(u2str(P)); USB_send(" (Pa) or ");
                float mm = P*0.0750062;
                USB_send(u2str(mm)); USB_send(" (*10mmHg)\n");
                if(H){
                    USB_send("H="); USB_send(u2str(H)); USB_send(" (*100%)\n");
                    showd(T/10, H/10);
                }
            }else USB_send("Can't read data\n");
        }else if(s == BMP280_ERR){
            USB_send("Error in measurement\n");
            BMP280_init();
        }

        char *txt, *ans;
        if((txt = get_USB())){
            IWDG->KR = IWDG_REFRESH;
            ans = (char*)parse_cmd(txt);
            if(ans){
                uint16_t l = 0; char *p = ans;
                while(*p++) l++;
                USB_send(ans);
            }
        }
    }
    return 0;
}
