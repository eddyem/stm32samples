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

static volatile uint8_t tx_succesfull = 1;
static volatile uint8_t rxNE = 0;

// interrupt IN handler (never used?)
static void EP1_Handler(){
    uint16_t epstatus = KEEP_DTOG(USB->EPnR[1]);
    if(RX_FLAG(epstatus)) epstatus = (epstatus & ~USB_EPnR_STAT_TX) ^ USB_EPnR_STAT_RX; // set valid RX
    else epstatus = epstatus & ~(USB_EPnR_STAT_TX|USB_EPnR_STAT_RX);
    // clear CTR
    epstatus = (epstatus & ~(USB_EPnR_CTR_RX|USB_EPnR_CTR_TX));
    USB->EPnR[1] = epstatus;
}

// data IN/OUT handlers
static void transmit_Handler(){ // EP3IN
    tx_succesfull = 1;
    uint16_t epstatus = KEEP_DTOG_STAT(USB->EPnR[3]);
    // clear CTR keep DTOGs & STATs
    USB->EPnR[3] = (epstatus & ~(USB_EPnR_CTR_TX)); // clear TX ctr
}

static void receive_Handler(){ // EP2OUT
    rxNE = 1;
    uint16_t epstatus = KEEP_DTOG_STAT(USB->EPnR[2]);
    USB->EPnR[2] = (epstatus & ~(USB_EPnR_CTR_RX)); // clear RX ctr
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
    // allow RESET and WKUPM interrupts
    USB->CNTR = USB_CNTR_RESETM | USB_CNTR_WKUPM;
    // clear flags
    USB->ISTR = 0;
    // and activate pullup
    USB->BCDR |= USB_BCDR_DPPU;
    NVIC_EnableIRQ(USB_IRQn);
}


static int usbwr(const uint8_t *buf, uint16_t l){
    uint32_t ctra = 1000000;
    while(--ctra && tx_succesfull == 0){
        IWDG->KR = IWDG_REFRESH;
    }
    tx_succesfull = 0;
    EP_Write(3, buf, l);
    ctra = 1000000;
    while(--ctra && tx_succesfull == 0){
        IWDG->KR = IWDG_REFRESH;
    }
    if(tx_succesfull == 0){usbON = 0; return 1;} // usb is OFF?
    return 0;
}

static uint8_t usbbuff[USB_TXBUFSZ-1]; // temporary buffer (63 - to prevent need of ZLP)
static uint8_t buflen = 0; // amount of symbols in usbbuff

// send next up to 63 bytes of data in usbbuff
static void send_next(){
    if(!buflen || !tx_succesfull) return;
    tx_succesfull = 0;
    EP_Write(3, usbbuff, buflen);
    buflen = 0;
}

// unblocking sending - just fill a buffer
void USB_send(const uint8_t *buf, uint16_t len){
    if(!usbON || !len) return;
    if(len > USB_TXBUFSZ-1 - buflen){
        usbwr(usbbuff, buflen);
        buflen = 0;
    }
    if(len > USB_TXBUFSZ-1){
        USB_send_blk(buf, len);
        return;
    }
    while(len--) usbbuff[buflen++] = *buf++;
}

// send zero-terminated string
void USB_sendstr(const char *str){
    uint16_t l = 0;
    const char *ptr = str;
    while(*ptr++) ++l;
    USB_send((uint8_t*)str, l);
}

// blocking sending
void USB_send_blk(const uint8_t *buf, uint16_t len){
    if(!usbON || !len) return; // USB disconnected
    if(buflen){
        usbwr(usbbuff, buflen);
        buflen = 0;
    }
    int needzlp = 0;
    while(len){
        if(len == USB_TXBUFSZ) needzlp = 1;
        uint16_t s = (len > USB_TXBUFSZ) ? USB_TXBUFSZ : len;
        if(usbwr(buf, s)) return;
        len -= s;
        buf += s;
    }
    if(needzlp){
        usbwr(NULL, 0);
    }
}

void usb_proc(){
    switch(USB_Dev.USB_Status){
        case USB_STATE_CONFIGURED:
            // make new BULK endpoint
            // Buffer have 1024 bytes, but last 256 we use for CAN bus (30.2 of RM: USB main features)
            EP_Init(1, EP_TYPE_INTERRUPT, USB_EP1BUFSZ, 0, EP1_Handler); // IN1 - transmit
            EP_Init(2, EP_TYPE_BULK, 0, USB_RXBUFSZ, receive_Handler); // OUT2 - receive data
            EP_Init(3, EP_TYPE_BULK, USB_TXBUFSZ, 0, transmit_Handler); // IN3 - transmit data
            USB_Dev.USB_Status = USB_STATE_CONNECTED;
        break;
        case USB_STATE_DEFAULT:
        case USB_STATE_ADDRESSED:
            if(usbON){
                usbON = 0;
            }
        break;
        default: // USB_STATE_CONNECTED - send next data portion
            if(!usbON) return;
            send_next();
    }
}

/**
 * @brief USB_receive
 * @param buf (i) - buffer[64] for received data
 * @return amount of received bytes
 */
uint8_t USB_receive(uint8_t *buf){
    if(!usbON || !rxNE) return 0;
    uint8_t sz = EP_Read(2, buf);
    uint16_t epstatus = KEEP_DTOG(USB->EPnR[2]);
    // keep stat_tx & set ACK rx
    USB->EPnR[2] = (epstatus & ~(USB_EPnR_STAT_TX)) ^ USB_EPnR_STAT_RX;
    rxNE = 0;
    return sz;
}

