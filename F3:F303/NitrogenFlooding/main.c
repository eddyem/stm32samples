/*
 * This file is part of the nitrogen project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "adc.h"
#include "BMP280.h"
//#include "buttons.h"
//#include "can.h"
//#include "flash.h"
#include "hardware.h"
#include "i2c.h"
#include "proto.h"
#include "strfunc.h"
#include "usb.h"

#define MAXSTRLEN    RBINSZ

volatile uint32_t Tms = 0;

void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    char inbuff[MAXSTRLEN+1];
    if(StartHSE()){
        SysTick_Config((uint32_t)72000); // 1ms
    }else{
        StartHSI();
        SysTick_Config((uint32_t)48000); // 1ms
    }
    USBPU_OFF(); // make a reconnection
    //flashstorage_init();
    hw_setup();
    USB_setup();
    //CAN_setup(the_conf.CANspeed);
    adc_setup();
    BMP280_setup(0);
    USBPU_ON();
    uint32_t ctr = 0;
    // CAN_message *can_mesg;
    while(1){
        IWDG->KR = IWDG_REFRESH;
        if(Tms - ctr > 499){
            ctr = Tms;
            LED_blink(0);
        }
        /*CAN_proc();
        if(CAN_get_status() == CAN_FIFO_OVERRUN){
            USB_sendstr("CAN bus fifo overrun occured!\n");
        }
        while((can_mesg = CAN_messagebuf_pop())){
            if(can_mesg && isgood(can_mesg->ID)){
                if(ShowMsgs){ // display message content
                    IWDG->KR = IWDG_REFRESH;
                    uint8_t len = can_mesg->length;
                    printu(Tms);
                    USB_sendstr(" #");
                    printuhex(can_mesg->ID);
                    for(uint8_t i = 0; i < len; ++i){
                        USB_putbyte(' ');
                        printuhex(can_mesg->data[i]);
                    }
                    USB_putbyte('\n');
                }
            }
        }*/
        if(I2C_scan_mode){
            uint8_t addr;
            int ok = i2c_scan_next_addr(&addr);
            if(addr == I2C_ADDREND) USND("Scan ends");
            else if(ok){
                USB_sendstr(uhex2str(addr));
                USB_sendstr(" ("); USB_sendstr(u2str(addr));
                USB_sendstr(") - found device\n");
            }
        }
        //i2c_have_DMA_Rx(); // check if there's DMA Rx complete
        BMP280_process();
        BMP280_status s = BMP280_get_status();
        if(s == BMP280_RDY){ // data ready - get it
            float T, P, H;
            if(BMP280_getdata(&T, &P, &H)){
                USB_sendstr("T="); USB_sendstr(float2str(T, 2)); USB_sendstr("\nP=");
                USB_sendstr(float2str(P, 1));
                P *= 0.00750062f; USB_sendstr("\nPmm="); USB_sendstr(float2str(P, 1));
                USB_sendstr("\nH="); USB_sendstr(float2str(H, 1));
                USB_sendstr("\nTdew="); USB_sendstr(float2str(Tdew(T, H), 1));
                newline();
            }else USB_sendstr("Can't read data\n");
        }else if(s == BMP280_ERR){
            USB_sendstr("BME280 error\n");
            BMP280_init();
        }
        int l = USB_receivestr(inbuff, MAXSTRLEN);
        if(l < 0) USB_sendstr("ERROR: USB buffer overflow or string was too long\n");
        else if(l){
            const char *ans = cmd_parser(inbuff);
            if(ans) USB_sendstr(ans);
        }
        //process_keys();
    }
}
