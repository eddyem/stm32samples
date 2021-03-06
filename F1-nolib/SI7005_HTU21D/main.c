/*
 * This file is part of the si7005 project.
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

#include "dewpoint.h"
#include "hardware.h"
#include "htu21d.h"
#include "proto.h"
#include "si7005.h"
#include "usb.h"
#include "usb_lib.h"

#define USBBUFSZ    127

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

// usb getline
char *get_USB(){
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

void showd(int32_t mT, int32_t mH){
    if(mT == -10000 || mH == -10000) return;
    USB_send("Dewpoint=");
    int32_t d = dewpoint(mT, mH);
    if(d < 0){
        USB_send("-");
        d = -d;
    }
    USB_send(u2str(d));
    USB_send("*10degrC\n");
}

int main(void){
    uint32_t lastT = 0;
    int32_t mT = -10000; // measured H & T for dewpoint
    int32_t mH = -10000;
    sysreset();
    StartHSE();
    hw_setup();
    si7005_setup();
    HTU21D_setup();
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
        si7005_process();
        HTU21D_process();
        HTU21D_status h = HTU21D_get_status();
        si7005_status s = si7005_get_status();

        if(s == SI7005_HRDY || h == HTU21D_HRDY){ // humidity can be shown
            if(s == SI7005_HRDY) mH = si7005_getH();
            else mH = HTU21D_getH();
            USB_send("Hum=");
            USB_send(u2str(mH));
            USB_send("*10%\n");
            showd(mT, mH);
        }else if(s == SI7005_TRDY || h == HTU21D_TRDY){ // T can be shown
            if(s == SI7005_TRDY) mT = si7005_getT();
            else mT = HTU21D_getT();
            int32_t T = mT;
            USB_send("T=");
            if(T < 0){
                USB_send("-");
                T = -T;
            }
            USB_send(u2str(T));
            USB_send("*10degrC\n");
            showd(mT, mH);
        }else if(s == SI7005_ERR || h == HTU21D_ERR){
            USB_send("Error in H/T measurement\n");
            si7005_setup();
            HTU21D_setup();
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
