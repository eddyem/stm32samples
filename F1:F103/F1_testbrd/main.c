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
#include "proto.h"
#include "usart.h"
#include "usb.h"
#include "usb_lib.h"

// ADC value threshold for meaning it is new
#define ADCthreshold    (20)

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

volatile uint8_t ADCmon = 0; // ==1 to monitor ADC (change PWM of LEDS & show current value)
uint16_t oldADCval = 0;

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
    USBPU_ON();

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(lastT > Tms || Tms - lastT > 499){
            if(ADCmon){
                uint16_t v = getADCval(0);
                int32_t d = v - oldADCval;
                if(d < -ADCthreshold || d > ADCthreshold){
                    oldADCval = v;
                    printADCvals();
                    v >>= 2; // 10 bits
                    TIM3->CCR1 = TIM3->CCR2 = TIM3->CCR3 = 0xff; TIM3->CCR4 = 0;
                    if(v >= 0x300) TIM3->CCR4 = v - 0x300;
                    else if(v >= 0x200) TIM3->CCR3 = v - 0x200;
                    else if(v >= 0x100){ TIM3->CCR2 = v - 0x100; TIM3->CCR3 = 0; }
                    else{ TIM3->CCR1 = v; TIM3->CCR2 = TIM3->CCR3 = 0; }
                }
            }
            lastT = Tms;
            transmit_tbuf(); // non-blocking transmission of data from UART buffer every 0.5s
        }
        /*
        if(I2C_scan_mode){
            uint8_t addr;
            int ok = i2c_scan_next_addr(&addr);
            if(addr == I2C_ADDREND) USND("Scan ends\n");
            else if(ok){
                USB_sendstr(uhex2str(addr));
                USND(" ("); USB_sendstr(u2str(addr));
                USND(") - found device\n");
            }
        }
        */
        usb_proc();
        int r = 0;
        char *txt, *ans;
        if((txt = get_USB())){
            ans = (char*)parse_cmd(txt);
            SEND("Received data over USB:\n");
            SEND(txt);
            newline();
            if(ans){
                uint16_t l = 0; char *p = ans;
                while(*p++) l++;
                USB_send((uint8_t*)ans, l);
                if(ans[l-1] != '\n') USND("\n");
            }
        }
        if(usartrx()){ // usart1 received data, store in in buffer
            r = usart_getline(&txt);
            if(r){
                txt[r] = 0;
                ans = (char*)parse_cmd(txt);
                if(ans){
                    usart_send(ans);
                    transmit_tbuf();
                }
                USND("Got string over USART:\n");
                USB_sendstr(txt);
            }
        }
    }
    return 0;
}

