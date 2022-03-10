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
#include "screen.h"
#include "usb.h"
#include "usb_lib.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

void iwdg_setup(){
    uint32_t tmout = 16000000;
    /* Enable the peripheral clock RTC */
    /* (1) Enable the LSI (40kHz) */
    /* (2) Wait while it is not ready */
    RCC->CSR |= RCC_CSR_LSION; /* (1) */
    while((RCC->CSR & RCC_CSR_LSIRDY) != RCC_CSR_LSIRDY){if(--tmout == 0) break;} /* (2) */
    /* Configure IWDG */
    /* (1) Activate IWDG (not needed if done in option bytes) */
    /* (2) Enable write access to IWDG registers */
    /* (3) Set prescaler by 64 (1.6ms for each tick) */
    /* (4) Set reload value to have a rollover each 2s */
    /* (5) Check if flags are reset */
    /* (6) Refresh counter */
    IWDG->KR = IWDG_START; /* (1) */
    IWDG->KR = IWDG_WRITE_ACCESS; /* (2) */
    IWDG->PR = IWDG_PR_PR_1; /* (3) */
    IWDG->RLR = 1250; /* (4) */
    tmout = 16000000;
    while(IWDG->SR){if(--tmout == 0) break;} /* (5) */
    IWDG->KR = IWDG_REFRESH; /* (6) */
}

static uint8_t countms = 0;

char *parse_cmd(char *buf){
    if(buf[1] != '\n'){
        PutStringAt(0, SCREEN_HEIGHT-1-curfont->baseline, buf);
        ConvertScreenBuf();
        return buf;
    }
    switch(*buf){
        case '0':
            ScreenOFF();
            FillScreen(0);
            ConvertScreenBuf();
            ShowScreen();
            return "Fill 0\n";
        break;
        case '1':
            ScreenOFF();
            FillScreen(1);
            ConvertScreenBuf();
            ShowScreen();
            return "Fill 1\n";
        break;
        case '2':
            choose_font(FONT14);
            return "Font14\n";
        break;
        case '3':
            choose_font(FONT16);
            return "Font16\n";
        break;
        case 'C':
            ScreenOFF();
            FillScreen(0);
            return "OK\n";
        case 'p':
            pin_toggle(USBPU_port, USBPU_pin);
            USB_send("USB pullup is ");
            if(pin_read(USBPU_port, USBPU_pin)) USB_send("off\n");
            else USB_send("on\n");
            return NULL;
        break;
        case 'R':
            USB_send("Soft reset\n");
            NVIC_SystemReset();
        break;
        case 'S':
            ShowScreen();
            return "OK\n";
        break;
        case 'W':
            USB_send("Wait for reboot\n");
            while(1){nop();};
        break;
        case 'Z':
            countms = 1;
            return "Start\n";
        break;
        case 'z':
            countms = 0;
            return "Stop\n";
        break;
        default: // help
            return
            "'0' - fill 0\n"
            "'1' - fill 1\n"
            "'2,3' - select font\n"
            "'C' - clear screen\n"
            "'p' - toggle USB pullup\n"
            "'R' - software reset\n"
            "'S' - show screen\n"
            "'W' - test watchdog\n"
            "'Zz' -start/stop counting ms\n"
            ;
        break;
    }
    return NULL;
}

// usb getline
char *get_USB(){
    static char tmpbuf[512], *curptr = tmpbuf;
    static int rest = 511;
    int x = USB_receive(curptr, rest);
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

int main(void){
    uint32_t lastT = 0, mscnt = 0, Tmscnt = 0;
    sysreset();
    StartHSE();
    SysTick_Config(72000);
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags

    hw_setup();
    USBPU_OFF();
    USB_setup();
    PutStringAt(0, SCREEN_HEIGHT-1-curfont->baseline, "Test string");
    ConvertScreenBuf();
    iwdg_setup();
    USBPU_ON();

    while (1){
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
                    FillScreen(0);
                    PutStringAt(0, SCREEN_HEIGHT-1-curfont->baseline, u2str(++mscnt));
                    ConvertScreenBuf();
                    ShowScreen();
                }
            }
        }else{
            mscnt = 0;
            Tmscnt = 0;
        }
        IWDG->KR = IWDG_REFRESH;
        process_screen();
        usb_proc();
        char *txt, *ans;
        if((txt = get_USB())){
            ans = parse_cmd(txt);
            if(ans) USB_send(ans);
        }
    }
    return 0;
}

