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
#include "usart.h"
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
/*
static usb_LineCoding new_lc;
static uint8_t lcchange = 0;
static void show_new_lc(){
    SEND("got new linecoding:");
    SEND(" baudrate="); printu(new_lc.dwDTERate);
    SEND(", charFormat="); printu(new_lc.bCharFormat);
    SEND(" (");
    switch(new_lc.bCharFormat){
        case USB_CDC_1_STOP_BITS:
            usart_putchar('1');
        break;
        case USB_CDC_1_5_STOP_BITS:
            SEND("1.5");
        break;
        case USB_CDC_2_STOP_BITS:
            usart_putchar('2');
        break;
        default:
            usart_putchar('?');
    }
    SEND(" stop bits), parityType="); printu(new_lc.bParityType);
    SEND(" (");
    switch(new_lc.bParityType){
        case USB_CDC_NO_PARITY:
            SEND("no");
        break;
        case USB_CDC_ODD_PARITY:
            SEND("odd");
        break;
        case USB_CDC_EVEN_PARITY:
            SEND("even");
        break;
        case USB_CDC_MARK_PARITY:
            SEND("mark");
        break;
        case USB_CDC_SPACE_PARITY:
            SEND("space");
        break;
        default:
            SEND("unknown");
    }
    SEND(" parity), dataBits="); printu(new_lc.bDataBits);
    usart_putchar('\n');
    lcchange = 0;
}

void linecoding_handler(usb_LineCoding *lc){
    memcpy(&new_lc, lc, sizeof(usb_LineCoding));
    lcchange = 1;
}

void clstate_handler(uint16_t val){
    SEND("change control line state to ");
    printu(val);
    if(val & CONTROL_DTR) SEND(" (DTR)");
    if(val & CONTROL_RTS) SEND(" (RTS)");
    usart_putchar('\n');
}
*/

// usb getline
char *get_USB(){
    static char tmpbuf[512], *curptr = tmpbuf;
    static int rest = 511;
    uint8_t x = USB_receive((uint8_t*)curptr);
    curptr[x] = 0;
    if(!x) return NULL;
    if(curptr[x-1] == '\n'){
#ifdef EBUG
        DBG("fullline");
        IWDG->KR = IWDG_REFRESH;
        SEND(tmpbuf);
        transmit_tbuf();
#endif
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

#define USND(str)  do{USB_send((uint8_t*)str, sizeof(str)-1);}while(0)
static const char *parse_cmd(const char *buf){
    if(buf[1] != '\n') return buf;
    switch(*buf){
        case 'L':
            USND("Very long test string for USB (it's length is more than 64 bytes).\n"
                     "This is another part of the string! Can you see all of this?\n");
            return "OK\n";
        break;
        case 'R':
            USND("Soft reset\n");
            NVIC_SystemReset();
        break;
        case 'S':
            USND("Test string for USB\n");
            return "OK\n";
        break;
        case 'W':
            USND("Wait for reboot\n");
            while(1){nop();};
        break;
        default: // help
            return
            "'L' - send long string over USB\n"
            "'R' - software reset\n"
            "'S' - send short string over USB\n"
            "'W' - test watchdog\n"
            ;
        break;
    }
    return NULL;
}

int main(void){
    uint32_t lastT = 0;
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup();
    usart_setup();

    MSG("Run");

    if(RCC->CSR & RCC_CSR_IWDGRSTF){ // watchdog reset occured
        SEND("WDGRESET=1\n");
    }
    if(RCC->CSR & RCC_CSR_SFTRSTF){ // software reset occured
        SEND("SOFTRESET=1\n");
    }
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags

    USB_setup();
    iwdg_setup();

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(lastT > Tms || Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
            transmit_tbuf(); // non-blocking transmission of data from UART buffer every 0.5s
        }
        usb_proc();
        char *txt, *ans;
        if(usartrx()){
            usart_getline(&txt);
            ans = (char*)parse_cmd(txt);
            if(ans) usart_send(ans);
        }
        if((txt = get_USB())){
            IWDG->KR = IWDG_REFRESH;
            ans = (char*)parse_cmd(txt);
            if(ans){
                uint16_t l = 0; char *p = ans;
                while(*p++) l++;
                USB_send((uint8_t*)ans, l);
            }
        }
        /*if(lcchange){
            show_new_lc();
        }*/
    }
    return 0;
}

