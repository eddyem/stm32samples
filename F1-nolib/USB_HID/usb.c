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

// incoming buffer size
#define IDATASZ     (256)
static volatile uint8_t tx_succesfull = 0;
static int8_t usbON = 0; // ==1 when USB fully configured

// interrupt IN handler
static uint16_t EP1_Handler(ep_t ep){
    if(ep.tx_flag){
        tx_succesfull = 1;
        ep.status = SET_VALID_RX(ep.status);
        ep.status = SET_VALID_TX(ep.status);
    }
    return ep.status;
}

void USB_setup(){
    NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
    NVIC_DisableIRQ(USB_HP_CAN1_TX_IRQn);
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;
    USB->CNTR   = USB_CNTR_FRES; // Force USB Reset
    for(uint32_t ctr = 0; ctr < 72000; ++ctr) nop(); // wait >1ms
    USB->CNTR   = 0;
    USB->BTABLE = 0;
    USB->DADDR  = 0;
    USB->ISTR   = 0;
    USB->CNTR   = USB_CNTR_RESETM | USB_CNTR_WKUPM; // allow only wakeup & reset interrupts
    NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
    NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn );
}

void usb_proc(){
    if(USB_GetState() == USB_CONFIGURE_STATE){ // USB configured - activate other endpoints
        if(!usbON){ // endpoints not activated
            EP_Init(1, EP_TYPE_INTERRUPT, USB_TXBUFSZ, 0, EP1_Handler); // IN1 - transmit
            usbON = 1;
        }
    }else{
        usbON = 0;
    }
}

void USB_send(uint8_t *buf, uint16_t size){
    if(!usbON) return;
    uint16_t ctr = 0;
    while(size){
        uint16_t s = (size > USB_KEYBOARD_REPORT_SIZE) ? USB_KEYBOARD_REPORT_SIZE : size;
        tx_succesfull = 0;
        EP_Write(1, (uint8_t*)&buf[ctr], s);
        uint32_t ctra = 1000000;
        while(--ctra && tx_succesfull == 0);
        if(!tx_succesfull){
            DBG("Error sending data!");
        }
        size -= s;
        ctr += s;
    }
}

/**
 * @brief USB_configured
 * @return 1 if USB is in configured state
 */
int USB_configured(){
    return usbON;
}
