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

//#include "adc.h"
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
    ++Tms;
    increment_timer();
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
    static char btns[] = "BTN0=0, BTN1=0, PPS=0\n";
    switch(*buf){
        case '0':
            LED_off();
        break;
        case '1':
            LED_on();
        break;
        case 'b':
            btns[5] = GET_BTN0() + '0';
            btns[13] = GET_BTN1() + '0';
            btns[20] = GET_PPS() + '0';
            return btns;
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

static void parse_USBCMD(char *cmd){
#define CMP(a,b)  cmpstr(a, b, sizeof(b)-1)
#define GETNUM(x) if(getnum(cmd+sizeof(x)-1, &N)) goto bad_number;
    static uint8_t conf_modified = 0;
    uint8_t succeed = 0;
    int32_t N;
    if(!cmd || !*cmd) return;
    if(*cmd == '?'){ // help
        USB_send("Commands:\n"
                 CMD_DISTMIN   " - min distance threshold (cm)\n"
                 CMD_DISTMAX   " - max distance threshold (cm)\n"
                 CMD_PRINTTIME " - print time\n"
                 CMD_STORECONF " - store new configuration in flash\n"
                 );
    }else if(CMP(cmd, CMD_PRINTTIME) == 0){
        USB_send(get_time(&current_time, get_millis()));
    }else if(CMP(cmd, CMD_DISTMIN) == 0){ // set low limit
        DBG("CMD_DISTMIN");
        GETNUM(CMD_DISTMIN);
        if(N < 0 || N > 0xffff) goto bad_number;
        if(the_conf.dist_min != (uint16_t)N){
            conf_modified = 1;
            the_conf.dist_min = (uint16_t) N;
            succeed = 1;
        }
    }else if(CMP(cmd, CMD_DISTMAX) == 0){ // set low limit
        DBG("CMD_DISTMAX");
        GETNUM(CMD_DISTMAX);
        if(N < 0 || N > 0xffff) goto bad_number;
        if(the_conf.dist_max != (uint16_t)N){
            conf_modified = 1;
            the_conf.dist_max = (uint16_t) N;
            succeed = 1;
        }
    }else if(CMP(cmd, CMD_STORECONF) == 0){ // store everything
        DBG("Store");
        if(conf_modified){
            if(store_userconf()){
                USB_send("Error: can't save data!\n");
            }else{
                conf_modified = 0;
                succeed = 1;
            }
        }
    }
    if(succeed) USB_send("Success!\n");
    return;
  bad_number:
    USB_send("Error: bad number!\n");
}

int main(void){
    uint32_t lastT = 0;
    sysreset();
    StartHSE();
    hw_setup();
    LED1_off();
    USBPU_OFF();
    usarts_setup();
    SysTick_Config(SYSTICK_DEFLOAD);
    SEND("Chronometer version " VERSION ".\n");
    if(RCC->CSR & RCC_CSR_IWDGRSTF){ // watchdog reset occured
        SEND("WDGRESET=1\n");
    }
    if(RCC->CSR & RCC_CSR_SFTRSTF){ // software reset occured
        SEND("SOFTRESET=1\n");
    }
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags

    USB_setup();
    iwdg_setup();
    USBPU_ON();
    // read data stored in flash
#ifdef EBUG
    SEND("Old config:\n");
    dump_userconf();
#endif
    //writeatend();
    get_userconf();
#ifdef EBUG
    SEND("New config:\n");
    dump_userconf();
#endif

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(lastT > Tms || Tms - lastT > 499){
            if(need2startseq) GPS_send_start_seq();
            LED_blink();
            if(GPS_status != GPS_VALID) LED1_blink();
            else LED1_on();
            lastT = Tms;
            if(usartrx(LIDAR_USART)){
                char *txt;
                if(usart_getline(LIDAR_USART, &txt)){
                    DBG("LIDAR:");
                    DBG(txt);
                }
            }
#if defined EBUG || defined USART1PROXY
            transmit_tbuf(1); // non-blocking transmission of data from UART buffer every 0.5s
#endif
            transmit_tbuf(GPS_USART);
            transmit_tbuf(LIDAR_USART);
        }
        usb_proc();
        int r = 0;
        char *txt;
        if((txt = get_USB())){
            parse_USBCMD(txt);
            DBG("Received data over USB:");
            DBG(txt);
            USB_send(txt); // echo all back
        }
#if defined EBUG || defined USART1PROXY
        if(usartrx(1)){ // usart1 received data, store in in buffer
            r = usart_getline(1, &txt);
            if(r){
                txt[r] = 0;
#ifdef EBUG
                char *ans = parse_cmd(txt);
                if(ans){
                    transmit_tbuf(1);
                    usart_send(1, ans);
                    transmit_tbuf(1);
                }
#else // USART1PROXY - send received data to GPS
                usart_send(GPS_USART, txt);
#endif
            }
        }
#endif
        if(usartrx(GPS_USART)){
            r = usart_getline(GPS_USART, &txt);
            if(r){
                txt[r] = 0;
                GPS_parse_answer(txt);
            }
        }
        if(usartrx(LIDAR_USART)){
            r = usart_getline(LIDAR_USART, &txt);
            if(r){
                parse_lidar_data(txt);
            }
        }
    }
    return 0;
}
