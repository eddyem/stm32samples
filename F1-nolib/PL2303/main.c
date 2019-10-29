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

#include "hardware.h"
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

char *parse_cmd(char *buf){
    if(buf[1] != '\n') return buf;
    switch(*buf){
        case 'p':
            pin_toggle(USBPU_port, USBPU_pin);
            USB_send("USB pullup is ");
            if(pin_read(USBPU_port, USBPU_pin)) USB_send("off\n");
            else USB_send("on\n");
            return NULL;
        break;
        case 'L':
            USB_send("Very long test string for USB (it's length is more than 64 bytes).\n"
                     "This is another part of the string! Can you see all of this?\n");
            return "OK\n";
        break;
        case 'R':
            USB_send("Soft reset\n");
            NVIC_SystemReset();
        break;
        case 'S':
            USB_send("Test string for USB\n");
            return "OK\n";
        break;
        case 'W':
            USB_send("Wait for reboot\n");
            while(1){nop();};
        break;
        default: // help
            return
            "'p' - toggle USB pullup\n"
            "'L' - send long string over USB\n"
            "'R' - software reset\n"
            "'S' - send short string over USB\n"
            "'W' - test watchdog\n"
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
    uint32_t lastT = 0;
    sysreset();
    StartHSE();
    hw_setup();
    SysTick_Config(72000);
/*
    if(RCC->CSR & RCC_CSR_IWDGRSTF){ // watchdog reset occured
        SEND("WDGRESET=1\n");
    }
    if(RCC->CSR & RCC_CSR_SFTRSTF){ // software reset occured
        SEND("SOFTRESET=1\n");
    }*/
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
        char *txt, *ans;
        if((txt = get_USB())){
            ans = parse_cmd(txt);
            if(ans) USB_send(ans);
        }
    }
    return 0;
}

