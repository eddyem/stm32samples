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
#include <string.h> // memcpy, memmove

// incoming buffer size
#define IDATASZ     (256)
static uint8_t incoming_data[IDATASZ];
static uint8_t ovfl = 0;
static uint16_t idatalen = 0;
static int8_t usbON = 0; // ==1 when USB fully configured

// interrupt IN handler (never used?)
static uint16_t EP1_Handler(ep_t ep){
    uint8_t ep0buf[11];
    if (ep.rx_flag){
        EP_Read(1, ep0buf);
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
    MSG("EP2\n");
    if(ep.rx_flag){
        int rd = ep.rx_cnt, rest = IDATASZ - idatalen;
        if(rd){
            if(rd <= rest){
                idatalen += EP_Read(2, &incoming_data[idatalen]);
                ovfl = 0;
            }else{
                ep.status = SET_NAK_RX(ep.status);
                ovfl = 1;
                return ep.status;
            }
        }
#ifdef EBUG
        SEND("receive ");
        printu(ep.rx_cnt);
        SEND(" bytes, idatalen=");
        printu(idatalen);
        SEND(" , rest=");
        printu(rest);
        incoming_data[idatalen] = 0;
        SEND(" , the buffer now:\n");
        SEND((char*)incoming_data);
        usart_putchar('\n');
#endif
        // end of transaction: clear DTOGs
        ep.status = CLEAR_DTOG_RX(ep.status);
        ep.status = CLEAR_DTOG_TX(ep.status);
        ep.status = SET_STALL_TX(ep.status);
    }else if (ep.tx_flag){
        ep.status = KEEP_STAT_TX(ep.status);
    }
    ep.status = SET_VALID_RX(ep.status);
    return ep.status;
}

void USB_setup(){
    RCC->APB1ENR |= RCC_APB1ENR_CRSEN | RCC_APB1ENR_USBEN; // enable CRS (hsi48 sync) & USB
    RCC->CFGR3 &= ~RCC_CFGR3_USBSW; // reset USB
    RCC->CR2 |= RCC_CR2_HSI48ON; // turn ON HSI48
    uint32_t tmout = 16000000;
    while(!(RCC->CR2 & RCC_CR2_HSI48RDY)){if(--tmout == 0) break;}
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;
    CRS->CFGR &= ~CRS_CFGR_SYNCSRC;
    CRS->CFGR |= CRS_CFGR_SYNCSRC_1; // USB SOF selected as sync source
    CRS->CR |= CRS_CR_AUTOTRIMEN; // enable auto trim
    CRS->CR |= CRS_CR_CEN; // enable freq counter & block CRS->CFGR as read-only
    RCC->CFGR |= RCC_CFGR_SW;
    // allow RESET and CTRM interrupts
    USB->CNTR = USB_CNTR_RESETM | USB_CNTR_CTRM;
    // clear flags
    USB->ISTR = 0;
    // and activate pullup
    USB->BCDR |= USB_BCDR_DPPU;
    NVIC_EnableIRQ(USB_IRQn);
}

void usb_proc(){
    if(USB_GetState() == USB_CONFIGURE_STATE){ // USB configured - activate other endpoints
        if(!usbON){ // endpoints not activated
            SEND("Configure endpoints\n");
            // make new BULK endpoint
            // Buffer have 1024 bytes, but last 256 we use for CAN bus (30.2 of RM: USB main features)
            EP_Init(1, EP_TYPE_INTERRUPT, 10, 0, EP1_Handler); // IN1 - transmit
            EP_Init(2, EP_TYPE_BULK, 0, USB_RXBUFSZ, EP23_Handler); // OUT2 - receive data
            EP_Init(3, EP_TYPE_BULK, USB_TXBUFSZ, 0, EP23_Handler); // IN3 - transmit data
            usbON = 1;
        }
    }else{
        usbON = 0;
    }
}

void USB_send(char *buf){
    uint16_t l = 0, ctr = 0;
    char *p = buf;
    while(*p++) ++l;
    while(l){
        uint16_t s = (l > USB_TXBUFSZ) ? USB_TXBUFSZ : l;
        EP_Write(3, (uint8_t*)&buf[ctr], s);
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
    if(!bufsize || !idatalen) return 0;
    USB->CNTR = 0;
    int sz = (idatalen > bufsize) ? bufsize : idatalen, rest = idatalen - sz;
    memcpy(buf, incoming_data, sz);
    if(rest > 0){
        memmove(incoming_data, &incoming_data[sz], rest);
        idatalen = rest;
    }else idatalen = 0;
    if(ovfl){
        EP23_Handler(endpoints[2]);
        uint16_t epstatus = USB->EPnR[2];
        epstatus = CLEAR_DTOG_RX(epstatus);
        epstatus = SET_VALID_RX(epstatus);
        USB->EPnR[2] = epstatus;
    }
    USB->CNTR = USB_CNTR_RESETM | USB_CNTR_CTRM;
    return sz;
}

/**
 * @brief USB_configured
 * @return 1 if USB is in configured state
 */
int USB_configured(){
    return usbON;
}
