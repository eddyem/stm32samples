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
#include "hardware.h"
#include "usart.h"
#include "usb.h"
#include "usb_lib.h"
#include <string.h> // memcpy

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
    static char btns[] = "BTN0=0, BTN1=0\n";
    if(buf[1] != '\n') return buf;
    uint8_t pwm;
    switch(*buf){
        case '+':
            pwm = GET_LED_PWM();
            if(pwm < 255){
                SET_LED_PWM(pwm+1);
                return u2str(GET_LED_PWM());
            }else return "MAX!\n";
        break;
        case '-':
            pwm = GET_LED_PWM();
            if(pwm > 0){
                SET_LED_PWM(pwm-1);
                return u2str(GET_LED_PWM());
            }else return "MIN!\n";
        break;
        case 'b':
            btns[5] = GET_BTN0() + '0';
            btns[13] = GET_BTN1() + '0';
            return btns;
        break;
        case 'g':
            return u2str(GET_LED_PWM());
        break;
        case 'A':
            return u2str(getADCval(0));
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
            return u2str(getMCUtemp());
        break;
        case 'V':
            return u2str(getVdd());
        break;
        case 'W':
            USB_send("Wait for reboot\n");
            SEND("Wait for reboot\n");
            while(1){nop();};
        break;
        default: // help
            return
            "'+'/'-' - increase/decrease PWM by 1\n"
            "'b' - get buttons's state\n"
            "'g' - get LED PWM value\n"
            "'A' - get ADC8 value\n"
            "'L' - send long string over USB\n"
            "'R' - software reset\n"
            "'S' - send short string over USB\n"
            "'T' - MCU temperature\n"
            "'V' - Vdd\n"
            "'W' - test watchdog\n"
            ;
        break;
    }
    return NULL;
}

// usb getline
char *get_USB(){
    static char tmpbuf[129], *curptr = tmpbuf;
    static int rest = 128;
    int x = USB_receive(curptr, rest);
    curptr[x] = 0;
    if(!x) return NULL;
    SEND("got: ");
    SEND(curptr);
    newline();
    if(curptr[x-1] == '\n'){
        curptr = tmpbuf;
        rest = 128;
        return tmpbuf;
    }
    curptr += x; rest -= x;
    if(rest <= 0){ // buffer overflow
        curptr = tmpbuf;
        rest = 128;
    }
    return NULL;
}

int main(void){
    uint32_t lastT = 0, lastB = 0;
    sysreset();
    SysTick_Config(6000, 1);
    hw_setup();
    usart_setup();

    SEND("Hello! I'm ready.\n");

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
        int r = 0;
        char *txt, *ans;
        if((txt = get_USB())){
            ans = parse_cmd(txt);
            SEND("Received data over USB:\n");
            SEND(txt);
            newline();
            if(ans) USB_send(ans);
        }
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
        // check buttons - each 50ms
        if(Tms - lastB > 49){
            /*static uint8_t oldbtn0 = 0, oldbtn1 = 0;
            uint8_t btn0 = GET_BTN0(), btn1 = GET_BTN1(), pwm = GET_LED_PWM();
            // both: set to middle
            if(oldbtn0 != btn0 && oldbtn1 != btn1){
                SET_LED_PWM(127);
            }else if(oldbtn0 != btn0){ // pressed/released
                oldbtn0 = btn0;
                if(pwm < 255) SET_LED_PWM(pwm+1);
            }else if(oldbtn1 != btn1){
                oldbtn1 = btn1;
                if(pwm > 0) SET_LED_PWM(pwm-1);
            }*/
            lastB = Tms;
            uint8_t btn0 = GET_BTN0(), btn1 = GET_BTN1(), pwm = GET_LED_PWM();
            // both: set to middle
            if(btn0 && btn1){
                SET_LED_PWM(127);
            }else if(btn0){
                if(pwm < 255) SET_LED_PWM(pwm+1);
            }else if(btn1){
                if(pwm > 0) SET_LED_PWM(pwm-1);
            }
        }
    }
    return 0;
}

