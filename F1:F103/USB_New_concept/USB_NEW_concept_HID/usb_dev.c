/*
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32f1.h>
#include <string.h>

#include "usart.h"
#include "usb_descr.h"
#include "usb_dev.h"

static volatile uint8_t tx_succesfull = 1;

static void EP1_Handler(){
    uint16_t epstatus = KEEP_DTOG(USB->EPnR[1]);
    if(RX_FLAG(epstatus)) epstatus = (epstatus & ~USB_EPnR_STAT_TX) ^ USB_EPnR_STAT_RX; // set valid RX
    else{
        tx_succesfull = 1;
        epstatus = epstatus & ~(USB_EPnR_STAT_TX|USB_EPnR_STAT_RX);
    }
    // clear CTR
    epstatus = (epstatus & ~(USB_EPnR_CTR_RX|USB_EPnR_CTR_TX));
    USB->EPnR[1] = epstatus;
}

int USB_send(uint8_t *buf, uint16_t size){
    if(!usbON || !size) return 0;
    uint16_t ctr = 0;
    while(size){
        uint16_t s = (size > USB_EP1BUFSZ) ? USB_EP1BUFSZ : size;
        tx_succesfull = 0;
        EP_Write(1, (uint8_t*)&buf[ctr], s);
        uint32_t ctra = 1000000;
        while(--ctra && tx_succesfull == 0){
            IWDG->KR = IWDG_REFRESH;
        }
        if(!tx_succesfull){
            DBG("Error sending data!");
            return ctr;
        }
        size -= s;
        ctr += s;
    }
    return ctr;
}

// USB is configured: setup endpoints
void set_configuration(){
    DBG("set_configuration()");
    EP_Init(1, EP_TYPE_INTERRUPT, USB_EP1BUFSZ, 0, EP1_Handler); // IN1 - transmit
}



