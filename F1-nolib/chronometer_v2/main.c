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

#if 0
char *parse_cmd(char *buf){
    int32_t N;
    static char btns[] = "BTN0=0, BTN1=0, BTN2=0, PPS=0\n";
    event_log l = {.elog_sz = sizeof(event_log), .trigno = 2};
    switch(*buf){
        case '0':
            LED1_off(); // LED1 off @dbg
        break;
        case '1':
            LED1_on(); // LED1 on @dbg
        break;
        case 'a':
            l.shottime.Time = current_time;
            l.shottime.millis = Timer;
            l.triglen = getADCval(1);
            if(store_log(&l)) SEND("Error storing");
            else SEND("Store OK");
            newline(1);
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
        case 'D':
            if(dump_log(0, -1)) DBG("Error dumping log: empty?");
        break;
        case 'p':
            pin_toggle(USBPU_port, USBPU_pin);
            SEND("USB pullup is ");
            if(pin_read(USBPU_port, USBPU_pin)) SEND("off");
            else SEND("on");
            newline(1);
        break;
        case 'G':
            SEND("LIDAR_DIST=");
            printu(1, last_lidar_dist);
            SEND(", LIDAR_STREN=");
            printu(1, last_lidar_stren);
            newline(1);
        break;
        case 'L':
            sendstring("Very long test string for USB (it's length is more than 64 bytes).\n"
                     "This is another part of the string! Can you see all of this?\n");
            return "Long test sent\n";
        break;
        case 'R':
            sendstring("Soft reset\n");
            SEND("Soft reset\n");
            NVIC_SystemReset();
        break;
        case 'S':
            sendstring("Test string for USB\n");
            return "Short test sent\n";
        break;
        case 'T':
            SEND(get_time(&current_time, get_millis()));
        break;
        case 'W':
            sendstring("Wait for reboot\n");
            SEND("Wait for reboot\n");
            while(1){nop();}
        break;
        default: // help
            if(buf[1] != '\n') return buf;
            return
            "0/1 - turn on/off LED1\n"
            "'a' - add test log record\n"
            "'b' - get buttons's state\n"
            "'c' - send cold start\n"
            "'d' - dump current user conf\n"
            "'D' - dump log\n"
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

#define USBBUF 63
// usb getline
static char *get_USB(){
    static char tmpbuf[USBBUF+1], *curptr = tmpbuf;
    static int rest = USBBUF;
    int x = USB_receive(curptr, rest);
    if(!x) return NULL;
    curptr[x] = 0;
    if(x == 1 && *curptr == 0x7f){ // backspace
        if(curptr > tmpbuf){
            --curptr;
            USB_send("\b \b");
        }
        return NULL;
    }
    USB_send(curptr); // echo
    if(curptr[x-1] == '\n'){ // || curptr[x-1] == '\r'){
        curptr = tmpbuf;
        rest = USBBUF;
        // omit empty lines
        if(tmpbuf[0] == '\n') return NULL;
        // and wrong empty lines
        if(tmpbuf[0] == '\r' && tmpbuf[1] == '\n') return NULL;
        return tmpbuf;
    }
    curptr += x; rest -= x;
    if(rest <= 0){ // buffer overflow
        sendstring("\nUSB buffer overflow!\n");
        curptr = tmpbuf;
        rest = USBBUF;
    }
    return NULL;
}

void linecoding_handler(usb_LineCoding __attribute__((unused)) *lc){ // get/set line coding
#ifdef EBUG
    SEND("Change speed to ");
    printu(1, lc->dwDTERate);
    newline(1);
#endif
}


static volatile uint8_t USBconn = 0;
uint8_t USB_connected = 0; // need for usb.c
void clstate_handler(uint16_t __attribute__((unused)) val){ // lesser bits of val: RTS|DTR
    USBconn = 1; // if == 1 -> send welcome message
    USB_connected = 1;
#if 0
    if(val & 2){
        DBG("RTS set");
        sendstring("RTS set\n");
    }
    if(val & 1){
        DBG("DTR set");
        sendstring("DTR set\n");
    }
#endif
}

void break_handler(){ // client disconnected
    DBG("Disconnected");
    USB_connected = 0;
}

int main(void){
    uint32_t lastT = 0;
    sysreset();
    StartHSE();
    SysTick_Config(SYSTICK_DEFCONF); // function SysTick_Config decrements argument!
    // !!! hw_setup() should be the first in setup stage
    hw_setup();
    USB_setup();
    USBPU_ON();
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
    // read data stored in flash
    flashstorage_init();
    usarts_setup(); // setup usarts after reading configuration
    iwdg_setup();

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(Timer > 499) LED_on(); // turn ON LED0 over 0.25s after PPS pulse
        if(USBconn && Tms > 100){ // USB connection
            USBconn = 0;
            sendstring("Chronometer version " VERSION ".\n");
        }
        // check if triggers that was recently shot are off now
        fillunshotms();
        if(Tms - lastT > 499){
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
            IWDG->KR = IWDG_REFRESH;
            transmit_tbuf(1); // non-blocking transmission of data from UART buffer every 0.5s
            transmit_tbuf(GPS_USART);
            transmit_tbuf(LIDAR_USART);
#ifdef EBUG
            static int32_t oldctr = 0;
            if(timecntr && timecntr != oldctr){
                oldctr = timecntr;
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
                usart_putchar(1, '\n');
                newline(1);
            }
#endif
        }
        IWDG->KR = IWDG_REFRESH;
        usb_proc();
        IWDG->KR = IWDG_REFRESH;
        int r = 0;
        char *txt = NULL;
        if((txt = get_USB())){
            IWDG->KR = IWDG_REFRESH;
            parse_CMD(txt);
        }
        if(usartrx(1)){ // usart1 received data, store it in buffer
            r = usart_getline(1, &txt);
            IWDG->KR = IWDG_REFRESH;
            if(r){
                txt[r] = 0;
                if(the_conf.defflags & FLAG_GPSPROXY){
                    usart_send(GPS_USART, txt);
                }else{ // UART1 is additive serial/bluetooth console
                    usart_send(1, txt);
                    if(*txt != '\n'){
                        parse_CMD(txt);
                    }
                }
            }
        }
        if(usartrx(GPS_USART)){
            IWDG->KR = IWDG_REFRESH;
            r = usart_getline(GPS_USART, &txt);
            if(r){
                txt[r] = 0;
                GPS_parse_answer(txt);
                if(the_conf.defflags & FLAG_GPSPROXY) usart_send(1, txt);
            }
        }
        if(usartrx(LIDAR_USART)){
            IWDG->KR = IWDG_REFRESH;
            r = usart_getline(LIDAR_USART, &txt);
            if(r){
                if(the_conf.defflags & FLAG_NOLIDAR){
                    usart_send(LIDAR_USART, txt);
                    if(*txt != '\n'){
                        parse_CMD(txt);
                    }
                }else
                    parse_lidar_data(txt);
            }
        }
        chk_buzzer(); // should we turn off buzzer?
        chkTrig1(); // check trigger without interrupt
    }
    return 0;
}
