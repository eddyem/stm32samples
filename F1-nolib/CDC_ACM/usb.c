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
#include "usb.h"
#include "usb_lib.h"
#include "usart.h"

// incoming buffer size
#define IDATASZ     (256)
static uint8_t incoming_data[IDATASZ];
static uint8_t ovfl = 0;
static uint16_t idatalen = 0;
static volatile uint8_t tx_succesfull = 0;

// interrupt IN handler (never used?)
static uint16_t EP1_Handler(ep_t ep){
    if (ep.rx_flag){
        ep.status = SET_VALID_TX(ep.status);
        ep.status = KEEP_STAT_RX(ep.status);
    }else if (ep.tx_flag){
        ep.status = SET_VALID_RX(ep.status);
        ep.status = SET_STALL_TX(ep.status);
    }
    return ep.status;
}

// data IN/OUT handler
static uint16_t EP23_Handler(ep_t ep){
    if(ep.rx_flag){
        int rd = ep.rx_cnt, rest = IDATASZ - idatalen;
        if(rd){
            if(rd <= rest){
                idatalen += EP_Read(2, (uint16_t*)&incoming_data[idatalen]);
                ovfl = 0;
            }else{
                ep.status = SET_NAK_RX(ep.status);
                ovfl = 1;
                return ep.status;
            }
        }
    }else if (ep.tx_flag){
        tx_succesfull = 1;
    }
    ep.status = SET_VALID_RX(ep.status);
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
    //NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn );
}

void usb_proc(){
    switch(USB_Dev.USB_Status){
        case USB_STATE_CONFIGURED:
            // make new BULK endpoint
            // Buffer have 1024 bytes, but last 256 we use for CAN bus (30.2 of RM: USB main features)
            EP_Init(1, EP_TYPE_INTERRUPT, USB_EP1BUFSZ, 0, EP1_Handler); // IN1 - transmit
            EP_Init(2, EP_TYPE_BULK, 0, USB_RXBUFSZ, EP23_Handler); // OUT2 - receive data
            EP_Init(3, EP_TYPE_BULK, USB_TXBUFSZ, 0, EP23_Handler); // IN3 - transmit data
            USB_Dev.USB_Status = USB_STATE_CONNECTED;
            DBG("Connected");
        break;
        case USB_STATE_DEFAULT:
        case USB_STATE_ADDRESSED:
            if(usbON){
                DBG("def/adr");
                usbON = 0;
            }
        default:
            return;
    }
}

void USB_send(const char *buf){
    if(!usbON) return; // USB disconnected
    uint16_t l = 0, ctr = 0;
    const char *p = buf;
    while(*p++) ++l;
    while(l){
        uint16_t s = (l > USB_TXBUFSZ) ? USB_TXBUFSZ : l;
        tx_succesfull = 0;
        EP_Write(3, (uint8_t*)&buf[ctr], s);
        uint32_t ctra = 1000000;
        while(--ctra && tx_succesfull == 0){
            IWDG->KR = IWDG_REFRESH;
        }
        if(tx_succesfull == 0){usbON = 0; DBG("USB disconnected"); return;} // usb is OFF?
        l -= s;
        ctr += s;
    }
}

/**
 * @brief USB_receive
 * @param buf (i) - buffer for received data
 * @param bufsize - its size
 * @return amount of received bytes
 */
int USB_receive(char *buf, int bufsize){
    if(bufsize < 1 || !idatalen) return 0;
    uint32_t oldcntr = USB->CNTR;
    USB->CNTR = 0;
    int sz = (idatalen > bufsize) ? bufsize : idatalen, rest = idatalen - sz;
    for(int i = 0; i < sz; ++i) buf[i] = incoming_data[i];
    if(rest > 0){
        uint8_t *ptr = &incoming_data[sz];
        for(int i = 0; i < rest; ++i) incoming_data[i] = *ptr++;
        idatalen = rest;
    }else idatalen = 0;
    if(ovfl){
        EP23_Handler(endpoints[2]);
        uint16_t epstatus = USB->EPnR[2];
        epstatus = CLEAR_DTOG_RX(epstatus);
        epstatus = SET_VALID_RX(epstatus);
        USB->EPnR[2] = epstatus;
    }
    USB->CNTR = oldcntr;
    return sz;
}

