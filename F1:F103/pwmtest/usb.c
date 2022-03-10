/*
 * This file is part of the pwmtest project.
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

//#include <string.h>

#include "usb.h"
#include "usb_lib.h"

static int sstrlen(const char *s){
    if(!s) return 0;
    int l = 0;
    while(*s++) ++l;
    return l;
}

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
    NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
    NVIC_DisableIRQ(USB_HP_CAN1_TX_IRQn);
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;
    USB->CNTR   = USB_CNTR_FRES; // Force USB Reset
    for(uint32_t ctr = 0; ctr < 72000; ++ctr) nop(); // wait >1ms
    //uint32_t ctr = 0;
    USB->CNTR   = 0;
    USB->BTABLE = 0;
    USB->DADDR  = 0;
    USB->ISTR   = 0;
    USB->CNTR   = USB_CNTR_RESETM | USB_CNTR_WKUPM; // allow only wakeup & reset interrupts
    NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
}

static int usbwr(const char *buf, uint16_t l){
    uint32_t ctra = 1000000;
    while(--ctra && tx_succesfull == 0){
        IWDG->KR = IWDG_REFRESH;
    }
    tx_succesfull = 0;
    EP_Write(3, (uint8_t*)buf, l);
    ctra = 1000000;
    while(--ctra && tx_succesfull == 0){
        IWDG->KR = IWDG_REFRESH;
    }
    if(tx_succesfull == 0){usbON = 0; return 1;} // usb is OFF?
    return 0;
}

static char usbbuff[USB_TXBUFSZ-1]; // temporary buffer (63 - to prevent need of ZLP)
static uint8_t buflen = 0; // amount of symbols in usbbuff

// send next up to 63 bytes of data in usbbuff
static void send_next(){
    if(!buflen || !tx_succesfull) return;
    tx_succesfull = 0;
    EP_Write(3, (uint8_t*)usbbuff, buflen);
    buflen = 0;
}

// blocking sending
static void _USB_send_blk(const char *buf, int len){
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

// unblocking sending - just fill a buffer
void USB_send(const char *buf){
    if(!usbON || !buf || !*buf) return;
    int len = sstrlen(buf);
    if(len > USB_TXBUFSZ-1 - buflen){
        usbwr(usbbuff, buflen);
        buflen = 0;
    }
    if(len > USB_TXBUFSZ-1){
        _USB_send_blk(buf, len);
        return;
    }
    while(len--) usbbuff[buflen++] = *buf++;
}

void USB_send_blk(const char *buf){
    int len = sstrlen(buf);
    _USB_send_blk(buf, len);
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
    uint8_t sz = EP_Read(2, (uint16_t*)buf);
    uint16_t epstatus = KEEP_DTOG(USB->EPnR[2]);
    // keep stat_tx & set ACK rx
    USB->EPnR[2] = (epstatus & ~(USB_EPnR_STAT_TX)) ^ USB_EPnR_STAT_RX;
    rxNE = 0;
    return sz;
}
