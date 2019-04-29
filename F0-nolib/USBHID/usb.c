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

static int8_t usbON = 0; // ==1 when USB fully configured

// interrupt IN handler (never used?)
static uint16_t EP1_Handler(ep_t ep){
    uint8_t epbuf[10];
    if (ep.rx_flag){
        MSG("EP1 OUT\n");
        EP_Read(1, epbuf);
        ep.status = SET_VALID_TX(ep.status);
        ep.status = KEEP_STAT_RX(ep.status);
    }else if (ep.tx_flag){
        MSG("EP1 IN\n");
        ep.status = SET_VALID_RX(ep.status);
        ep.status = SET_STALL_TX(ep.status);
    }
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
            EP_Init(1, EP_TYPE_INTERRUPT, 10, 10, EP1_Handler); // IN1 - transmit
            usbON = 1;
        }
    }else{
        usbON = 0;
    }
}

void USB_send(uint8_t *buf, uint16_t size){
    uint16_t ctr = 0;
    while(size){
        uint16_t s = (size > USB_TXBUFSZ) ? USB_TXBUFSZ : size;
        EP_Write(1, (uint8_t*)&buf[ctr], s);
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
