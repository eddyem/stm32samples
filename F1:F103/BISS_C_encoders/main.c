/*
 * This file is part of the I2Cscan project.
 * Copyright 2020 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "flash.h"
#include "hardware.h"
#include "proto.h"
#include "spi.h"
#include "strfunc.h"
#include "usart.h"
#include "usb_dev.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

int main(){
    char inbuff[RBINSZ];
    uint32_t lastT = 0, lastS = 0;
    uint8_t encbuf[ENCODER_BUFSZ];
    StartHSE();
    flashstorage_init();
    hw_setup();
    USBPU_OFF();
    SysTick_Config(72000);
#ifdef EBUG
    usart_setup();
    DBG("Start");
    uint32_t tt = 0;
#endif
    USB_setup();
#ifndef EBUG
    iwdg_setup();
#endif
    USBPU_ON();
    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
        }
#ifdef EBUG
        if(Tms != tt){
            __disable_irq();
            usart_transmit();
            tt = Tms;
            __enable_irq();
        }
#endif
        if(CDCready[I_CMD]){
            if(Tms - lastS > 9999){
                USB_sendstr(I_CMD, "Tms=");
                USB_sendstr(I_CMD, u2str(Tms));
                newline(I_CMD);
                lastS = Tms;
            }
            int l = USB_receivestr(I_CMD, inbuff, RBINSZ);
            if(l < 0) CMDWRn("ERROR: CMD USB buffer overflow or string was too long");
            else if(l) parse_cmd(inbuff);
        }
        if(CDCready[I_X]){
            int l = USB_receivestr(I_X, inbuff, RBINSZ);
            if(l < 0) CMDWRn("ERROR: encX USB buffer overflow or string was too long");
            else if(l){
                CMDWR("EncX got: '");
                CMDWR(inbuff);
                CMDWR("'\n");
            }
            if(spi_read_enc(0, encbuf)){ // send encoder data
                hexdump(I_X, encbuf, ENCODER_BUFSZ);
            }
        }
        if(CDCready[I_Y]){
            int l = USB_receivestr(I_Y, inbuff, RBINSZ);
            if(l < 0) CMDWRn("ERROR: encY USB buffer overflow or string was too long");
            else if(l){
                CMDWR("EncY got: '");
                CMDWR(inbuff);
                CMDWR("'\n");
            }
            if(spi_read_enc(1, encbuf)){ // send encoder data
                hexdump(I_Y, encbuf, ENCODER_BUFSZ);
            }
        }
    }
    return 0;
}
