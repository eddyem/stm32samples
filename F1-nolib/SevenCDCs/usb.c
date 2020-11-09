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

static volatile uint8_t tx_succesfull[8] = {1,1,1,1,1,1,1,1}; // zero's idx is omit
static uint8_t rxNE[8] = {0}; // zero's idx is omit

uint8_t dontans[8] = {0};

static void rxtx_Handler(uint8_t epno){
    uint16_t epstatus = KEEP_DTOG(USB->EPnR[epno]);
    if(RX_FLAG(epstatus)){
        rxNE[epno] = 1;
        epstatus = (epstatus & ~(USB_EPnR_STAT_TX|USB_EPnR_CTR_RX)) ^ USB_EPnR_STAT_RX; // keep stat Tx & set valid RX, clear CTR Rx
    }else{
        tx_succesfull[epno] = 1;
        epstatus = (epstatus & ~(USB_EPnR_STAT_TX|USB_EPnR_CTR_TX)) ^ USB_EPnR_STAT_RX; // keep stat Tx & set valid RX, clear CTR Tx
    }
    USB->EPnR[epno] = epstatus;
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
}

static int usbwr(uint8_t idx, const uint8_t *buf, uint16_t l){
    uint32_t ctra = 200000;
    while(--ctra && tx_succesfull[idx] == 0){
        IWDG->KR = IWDG_REFRESH;
    }
    if(tx_succesfull[idx] == 0){
        dontans[idx] = 1;
        return 1;
    }else dontans[idx] = 0;
    tx_succesfull[idx] = 0;
    EP_Write(idx, buf, l);
    /*
    ctra = 1000000;
    while(--ctra && tx_succesfull[idx] == 0){
        IWDG->KR = IWDG_REFRESH;
    }
    //if(tx_succesfull[idx] == 0){usbON = 0; return 1;} // usb is OFF?
    if(tx_succesfull[idx] == 0){return 1;} // this channel is OFF?
    */
    return 0;
}

static uint8_t usbbuff[8][USB_TXBUFSZ-1]; // temporary buffer (63 - to prevent need of ZLP)
static uint8_t buflen[8] = {0}; // amount of symbols in usbbuff

// send next up to 64 bytes of data in usbbuff
static void send_next(){
    for(uint8_t idx = 1; idx < 8; ++idx){
        if(!buflen[idx] || !tx_succesfull[idx]) continue;
        tx_succesfull[idx] = 0;
        EP_Write(idx, usbbuff[idx], buflen[idx]);
        buflen[idx] = 0;
    }
}

// unblocking sending - just fill a buffer
void USB_send(uint8_t idx, const uint8_t *buf, uint16_t len){
    if(!usbON || !len) return;
    if(len > USB_TXBUFSZ-1 - buflen[idx]){
        usbwr(idx, usbbuff[idx], buflen[idx]);
        buflen[idx] = 0;
    }
    if(len > USB_TXBUFSZ-1){
        USB_send_blk(idx, buf, len);
        return;
    }
    while(len--) usbbuff[idx][buflen[idx]++] = *buf++;
}

// send zero-terminated string
void USB_sendstr(uint8_t idx, const char *str){
    uint16_t l = 0;
    const char *ptr = str;
    while(*ptr++) ++l;
    USB_send(idx, (uint8_t*)str, l);
}

// blocking sending
void USB_send_blk(uint8_t idx, const uint8_t *buf, uint16_t len){
    if(!usbON || !len) return; // USB disconnected
    if(buflen[idx]){
        usbwr(idx, usbbuff[idx], buflen[idx]);
        buflen[idx] = 0;
    }
    int needzlp = 0;
    while(len){
        if(len == USB_TXBUFSZ) needzlp = 1;
        uint16_t s = (len > USB_TXBUFSZ) ? USB_TXBUFSZ : len;
        if(usbwr(idx, buf, s)) return;
        len -= s;
        buf += s;
    }  
    if(needzlp){
        usbwr(idx, NULL, 0);
    }
}

uint8_t ifacesno = 0, errcode = 0;

void usb_proc(){
    switch(USB_Dev.USB_Status){
        case USB_STATE_CONFIGURED:
            // make new BULK endpoint
            // Buffer have 1024 bytes, but last 256 we use for CAN bus (30.2 of RM: USB main features)
            //EP_Init(1, EP_TYPE_INTERRUPT, USB_EP1BUFSZ, 0, EP1_Handler); // IN1 - transmit
            ifacesno = 0;
            for(uint8_t i = 1; i < 8; ++i){
                if((errcode = EP_Init(i, EP_TYPE_BULK, USB_TXBUFSZ, USB_RXBUFSZ, rxtx_Handler))) break;
                ++ifacesno;
            }
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
uint8_t USB_receive(uint8_t idx, uint8_t *buf){
    if(idx < 1 || idx > 7) return 0;
    if(!usbON || !rxNE[idx]) return 0;
    uint8_t sz = EP_Read(idx, (uint16_t*)buf);
    uint16_t epstatus = KEEP_DTOG(USB->EPnR[idx]);
    // keep stat_tx & set ACK rx
    USB->EPnR[idx] = (epstatus & ~(USB_EPnR_STAT_TX)) ^ USB_EPnR_STAT_RX;
    rxNE[idx] = 0;
    return sz;
}
