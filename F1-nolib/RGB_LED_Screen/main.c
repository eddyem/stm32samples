/*
 * main.c
 *
 * Copyright 2017 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "fonts.h"
#include "hardware.h"
#include "proto.h"
#include "screen.h"
#include "usb.h"
#include "usb_lib.h"

volatile uint32_t Tms = 0;
uint8_t countms = 0, rainbow = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

#define USBBUFSZ    (127)
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

static void nxtrainbow(){
    for(int i = 0; i < SCREEN_WIDTH; ++i){
        for(int j = 0; j < SCREEN_HEIGHT; ++j){
            DrawPix(i, j, rainbow + i);
        }
    }
    if(++rainbow == 0) rainbow = 1;
}

int main(void){
    uint32_t lastT = 0, mscnt = 0, Tmscnt = 0, lastR = 0;
    sysreset();
    StartHSE();
    SysTick_Config(72000);
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags

    hw_setup();
    USBPU_OFF();
    USB_setup();
    PutStringAt(1, SCREEN_HEIGHT-1-curfont->baseline, "Test string");
    iwdg_setup();
    USBPU_ON();

    while(1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
        }
        if(countms){
            if(!Tmscnt){
                Tmscnt = Tms;
                if(!Tmscnt) Tmscnt = 1;
            }else{
                if(Tms - Tmscnt > 99){
                    Tmscnt = Tms;
                    ClearScreen();
                    PutStringAt(5, curfont->height + 1, u2str(++mscnt));
                    ScreenON();
                }
            }
        }else{
            mscnt = 0;
            Tmscnt = 0;
        }
        if(rainbow && Tms - lastR > 99){
            lastR = Tms;
            nxtrainbow();
        }
        IWDG->KR = IWDG_REFRESH;
        process_screen();
        usb_proc();
        char *txt; const char *ans;
        if((txt = get_USB())){
            ans = parse_cmd(txt);
            if(ans) USB_send(ans);
        }
    }
    return 0;
}

