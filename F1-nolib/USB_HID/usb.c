/*
 *                                                                                                  geany_encoding=koi8-r
 * usb.c - base functions for different USB types
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 *
 */
#include "keycodes.h"
#include "usb.h"
#include "usb_lib.h"
#include "usart.h"

static volatile uint8_t tx_succesfull = 1;

// interrupt IN handler
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

void USB_setup(){
    NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
    NVIC_DisableIRQ(USB_HP_CAN1_TX_IRQn);
    DBG("USB setup");
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;
    USB->CNTR   = USB_CNTR_FRES; // Force USB Reset
    for(uint32_t ctr = 0; ctr < 72000; ++ctr) nop(); // wait >1ms
    USB->CNTR   = 0;
    USB->BTABLE = 0;
    USB->DADDR  = 0;
    USB->ISTR   = 0;
    USB->CNTR   = USB_CNTR_RESETM | USB_CNTR_WKUPM; // allow only wakeup & reset interrupts
    NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
}

void usb_proc(){
    if(USB_Dev.USB_Status == USB_STATE_CONFIGURED){ // USB configured - activate other endpoints
        if(!usbON){ // endpoints not activated
            EP_Init(1, EP_TYPE_INTERRUPT, USB_TXBUFSZ, 0, EP1_Handler); // IN1 - transmit
            usbON = 1;
        }
    }else{
        usbON = 0;
    }
}

void USB_send(uint8_t *buf, uint16_t size){
    if(!usbON || !size) return;
    uint16_t ctr = 0;
    while(size){
        uint16_t s = (size > USB_KEYBOARD_REPORT_SIZE) ? USB_KEYBOARD_REPORT_SIZE : size;
        tx_succesfull = 0;
        EP_Write(1, (uint8_t*)&buf[ctr], s);
        uint32_t ctra = 1000000;
        while(--ctra && tx_succesfull == 0){
            IWDG->KR = IWDG_REFRESH;
        }
        if(!tx_succesfull){
            DBG("Error sending data!");
        }
        size -= s;
        ctr += s;
    }
}

