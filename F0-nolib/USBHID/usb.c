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
    if(!usbON) return;
    uint16_t ctr = 0;
    while(size){
        uint16_t s = (size > USB_KEYBOARD_REPORT_SIZE) ? USB_KEYBOARD_REPORT_SIZE : size;
        tx_succesfull = 0;
        EP_Write(1, (uint8_t*)&buf[ctr], s);
        uint32_t ctra = 1000000;
        while(--ctra && tx_succesfull == 0){
            IWDG->KR = IWDG_REFRESH;
        }
        if(!tx_succesfull) return;
        size -= s;
        ctr += s;
    }
}
