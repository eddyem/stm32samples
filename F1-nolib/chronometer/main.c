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

#include "adc.h"
#include "GPS.h"
#include "flash.h"
#include "hardware.h"
#include "lidar.h"
#include "str.h"
#include "time.h"
#include "usart.h"
#include "usb.h"
#include "usb_lib.h"

#ifndef VERSION
#define VERSION "0.0.0"
#endif

// global pseudo-milliseconds counter
volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms; // increment pseudo-milliseconds counter
    if(++Timer == 1000){ // increment milliseconds counter
        time_increment();
    }
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

#ifdef EBUG
char *parse_cmd(char *buf){
    int32_t N;
    static char btns[] = "BTN0=0, BTN1=0, BTN2=0, PPS=0\n";
    switch(*buf){
        case '0':
            LED_off(); // LED0 off @dbg
        break;
        case '1':
            LED_on(); // LED0 on @dbg
        break;
        case 'b':
            btns[5] = gettrig(0) + '0';
            btns[13] = gettrig(1) + '0';
            btns[21] = gettrig(2) + '0';
            btns[28] = GET_PPS() + '0';
            return btns;
        break;
        case 'c':
            DBG("Send cold start");
            GPS_send_FullColdStart();
        break;
        case 'C':
            if(getnum(&buf[1], &N)){
                SEND("Need a number!\n");
            }else{
                addNrecs(N);
            }
        break;
        case 'd':
            dump_userconf();
        break;
        case 'p':
            pin_toggle(USBPU_port, USBPU_pin);
            SEND("USB pullup is ");
            if(pin_read(USBPU_port, USBPU_pin)) SEND("off");
            else SEND("on");
            newline();
        break;
        case 'G':
            SEND("LIDAR_DIST=");
            printu(1, last_lidar_dist);
            SEND(", LIDAR_STREN=");
            printu(1, last_lidar_stren);
            newline();
        break;
        case 'L':
            USB_send("Very long test string for USB (it's length is more than 64 bytes).\n"
                     "This is another part of the string! Can you see all of this?\n");
            return "Long test sent\n";
        break;
        case 'R':
            USB_send("Soft reset\n");
            SEND("Soft reset\n");
            NVIC_SystemReset();
        break;
        case 'S':
            USB_send("Test string for USB\n");
            return "Short test sent\n";
        break;
        case 'T':
            SEND(get_time(&current_time, get_millis()));
        break;
        case 'W':
            USB_send("Wait for reboot\n");
            SEND("Wait for reboot\n");
            while(1){nop();};
        break;
        default: // help
            if(buf[1] != '\n') return buf;
            return
            "0/1 - turn on/off LED1\n"
            "'b' - get buttons's state\n"
            "'c' - send cold start\n"
            "'d' - dump current user conf\n"
            "'p' - toggle USB pullup\n"
            "'C' - store userconf for N times\n"
            "'G' - get last LIDAR distance\n"
            "'L' - send long string over USB\n"
            "'R' - software reset\n"
            "'S' - send short string over USB\n"
            "'T' - show current GPS time\n"
            "'W' - test watchdog\n"
            ;
        break;
    }
    return NULL;
}
#endif

// usb getline
static char *get_USB(){
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
        SEND("USB buffer overflow!\n");
        curptr = tmpbuf;
        rest = 511;
    }
    return NULL;
}

void linecoding_handler(usb_LineCoding __attribute__((unused)) *lc){ // get/set line coding
#ifdef EBUG
    SEND("Change speed to");
    printu(1, lc->dwDTERate);
    newline();
#endif
}

void clstate_handler(uint16_t __attribute__((unused)) val){ // lesser bits of val: RTS|DTR
    static uint32_t Tlast = 0;
    SEND("Tms/Tlast: ");
    printu(1, Tms);
    newline();
    printu(1, Tlast);
    newline();
    if(Tms - Tlast < 500) return;
    Tlast = Tms;
    USB_send("Chronometer version " VERSION ".\n");
#ifdef EBUG
    if(val & 2){
        DBG("RTS set");
        USB_send("RTS set\n");
    }
    if(val & 1){
        DBG("DTR set");
        USB_send("DTR set\n");
    }
#endif
}

void break_handler(){ // client disconnected
    DBG("Disconnected");
}

#ifdef EBUG
extern int32_t ticksdiff, timecntr, timerval, Tms1;
extern uint32_t last_corr_time;
#endif

int main(void){
    uint32_t lastT = 0;
    sysreset();
    StartHSE();
    SysTick_Config(SYSTICK_DEFCONF); // function SysTick_Config decrements argument!
    // read data stored in flash
    get_userconf();
    // !!! hw_setup() should be the first in setup stage
    hw_setup();
    USB_setup();
    USBPU_ON();
    usarts_setup();
#ifdef EBUG
    SEND("This is chronometer version " VERSION ".\n");
    if(RCC->CSR & RCC_CSR_IWDGRSTF){ // watchdog reset occured
        SEND("WDGRESET=1\n");
    }
    if(RCC->CSR & RCC_CSR_SFTRSTF){ // software reset occured
        SEND("SOFTRESET=1\n");
    }
#endif
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags
    //iwdg_setup();

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(Timer > 499) LED_on(); // turn ON LED0 over 0.25s after PPS pulse
        if(BuzzerTime && Tms - BuzzerTime > 249){
            BUZZER_OFF();
            BuzzerTime = 0;
        }
        if(lastT > Tms || Tms - lastT > 499){
            if(need2startseq) GPS_send_start_seq();
            IWDG->KR = IWDG_REFRESH;
            switch(GPS_status){
                case GPS_VALID:
                    LED1_blink(); // blink LED1 @ VALID time
                break;
                case GPS_NOT_VALID:
                    LED1_on(); // shine LED1 @ NON-VALID time
                break;
                default:
                    LED1_off(); // turn off LED1 if GPS not found or time unknown
            }
            lastT = Tms;
            if(usartrx(LIDAR_USART)){
                char *txt;
                if(usart_getline(LIDAR_USART, &txt)){
                    DBG("LIDAR:");
                    DBG(txt);
                }
            }
            IWDG->KR = IWDG_REFRESH;
#if defined EBUG || defined USART1PROXY
            transmit_tbuf(1); // non-blocking transmission of data from UART buffer every 0.5s
#endif
            transmit_tbuf(GPS_USART);
            transmit_tbuf(LIDAR_USART);
#ifdef EBUG
            static uint8_t x = 1;
            if(timecntr){
                if(x){
                    SEND("ticksdiff=");
                    if(ticksdiff < 0){
                        SEND("-");
                        printu(1, -ticksdiff);
                    }else printu(1, ticksdiff);
                    SEND(", timecntr=");
                    printu(1, timecntr);
                    SEND("\nlast_corr_time=");
                    printu(1, last_corr_time);
                    SEND(", Tms=");
                    printu(1, Tms1);
                    SEND("\nTimer=");
                    printu(1, timerval);
                    SEND(", LOAD=");
                    printu(1, SysTick->LOAD);
                    newline();
                }
                x = !x;
            }
#endif
        }
        IWDG->KR = IWDG_REFRESH;
        if(trigger_shot) show_trigger_shot(trigger_shot);
        IWDG->KR = IWDG_REFRESH;
        usb_proc();
        IWDG->KR = IWDG_REFRESH;
        int r = 0;
        char *txt;
        if((txt = get_USB())){
            DBG("Received data over USB:");
            DBG(txt);
            if(parse_USBCMD(txt))
                USB_send(txt); // echo back non-commands data
            IWDG->KR = IWDG_REFRESH;
        }
#if defined EBUG || defined USART1PROXY
        if(usartrx(1)){ // usart1 received data, store in in buffer
            r = usart_getline(1, &txt);
            if(r){
                txt[r] = 0;
#ifdef EBUG
                char *ans = parse_cmd(txt);
                IWDG->KR = IWDG_REFRESH;
                if(ans){
                    transmit_tbuf(1);
                    IWDG->KR = IWDG_REFRESH;
                    usart_send(1, ans);
                    transmit_tbuf(1);
                    IWDG->KR = IWDG_REFRESH;
                }
#else // USART1PROXY - send received data to GPS
                usart_send(GPS_USART, txt);
                IWDG->KR = IWDG_REFRESH;
#endif
            }
        }
#endif
        if(usartrx(GPS_USART)){
            IWDG->KR = IWDG_REFRESH;
            r = usart_getline(GPS_USART, &txt);
            if(r){
                txt[r] = 0;
                GPS_parse_answer(txt);
            }
        }
        if(usartrx(LIDAR_USART)){
            IWDG->KR = IWDG_REFRESH;
            r = usart_getline(LIDAR_USART, &txt);
            if(r){
                parse_lidar_data(txt);
            }
        }
        chkADCtrigger();
    }
    return 0;
}
