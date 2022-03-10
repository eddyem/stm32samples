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
#include "keycodes.h"
#include "usart.h"
#include "usb.h"
#include "usb_lib.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

static void hw_setup(){
    // Enable clocks to the GPIO subsystems (PB for ADC), turn on AFIO clocking to disable SWD/JTAG
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
    // turn off SWJ/JTAG
    //AFIO->MAPR = AFIO_MAPR_SWJ_CFG_DISABLE;
    // turn off USB pullup
    GPIOA->ODR = (1<<13);
    // Set led (PC13) as opendrain output
    GPIOC->CRH = CRH(13, CNF_ODOUTPUT|MODE_SLOW);
    // Set USB pullup (PA13) as opendrain output
    GPIOA->CRH = CRH(13, CNF_ODOUTPUT|MODE_SLOW);
}

static void iwdg_setup(){
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
 * buf[0]: 1 - report ID
 * buf[1]: bit2 - middle button, bit1 - right, bit0 - left
 * buf[2]: move X
 * buf[3]: move Y
 * buf[4]: wheel
 */
static void move_mouse(int8_t x, int8_t y){
    int8_t buf[5] = {1,0,0,0,0};
    buf[2] = x; buf[3] = y;
    USB_send((uint8_t*)buf, 5);
}

/*
 * Keyboard buffer:
 * buf[1]: MOD
 * buf[2]: reserved
 * buf[3]..buf[8] - keycodes 1..6
 */
static void send_word(const char *wrd){
    char last = 0;
    do{
        // release key before pressing it again
        if(last == *wrd) USB_send(release_key(), USB_KEYBOARD_REPORT_SIZE);
        last = *wrd;
        USB_send(press_key(last), USB_KEYBOARD_REPORT_SIZE);
        IWDG->KR = IWDG_REFRESH;
    }while(*(++wrd));
    USB_send(release_key(), USB_KEYBOARD_REPORT_SIZE);
}

static char *parse_cmd(char *buf){
    if(buf[1] != '\n') return buf;
    switch(*buf){
        case 'p':
            pin_toggle(USBPU_port, USBPU_pin);
            SEND("USB pullup is ");
            if(pin_read(USBPU_port, USBPU_pin)) SEND("off");
            else SEND("on");
            newline();
        break;
        case 'C':
            SEND("USB ");
            if(!usbON) SEND("dis");
            SEND("connected\n");
        break;
        case 'K':
            send_word("Hello, weird and cruel world!\n\n");
            SEND("Write hello\n");
        break;
        case 'M':
            move_mouse(100, 10);
            move_mouse(0,0);
            SEND("Move mouse\n");
        break;
        case 'R':
            SEND("Soft reset\n");
            NVIC_SystemReset();
        break;
        case 'W':
            SEND("Wait for reboot\n");
            while(1){nop();};
        break;
        default: // help
            return
            "'p' - toggle USB pullup\n"
            "'C' - test if USB is configured\n"
            "'K' - emulate keyboard\n"
            "'M' - move mouse\n"
            "'R' - software reset\n"
            "'W' - test watchdog\n"
            ;
        break;
    }
    return NULL;
}


int main(void){
    uint32_t lastT = 0;
    sysreset();
    StartHSE();
    hw_setup();
    usart_setup();
    SysTick_Config(72000);

    SEND("Hello! I'm ready.\n");

    if(RCC->CSR & RCC_CSR_IWDGRSTF){ // watchdog reset occured
        SEND("WDGRESET=1\n");
    }
    if(RCC->CSR & RCC_CSR_SFTRSTF){ // software reset occured
        SEND("SOFTRESET=1\n");
    }
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags

    USBPU_OFF();
    USB_setup();
    iwdg_setup();
    USBPU_ON();

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(lastT > Tms || Tms - lastT > 499){
            LED_blink();
            lastT = Tms;
            transmit_tbuf(); // non-blocking transmission of data from UART buffer every 0.5s
        }
        usb_proc();
        int r = 0;
        char *txt, *ans;
        if(usartrx()){ // usart1 received data, store in in buffer
            r = usart_getline(&txt);
            if(r){
                txt[r] = 0;
                ans = parse_cmd(txt);
                if(ans){
                    usart_send(ans);
                    transmit_tbuf();
                }
            }
        }
    }
    return 0;
}

