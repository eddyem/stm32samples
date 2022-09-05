/*
 * This file is part of the MLX90640 project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "ringbuffer.h"
#include "usb.h"
#include "usb_lib.h"

static char usbbuff[USB_TXBUFSZ]; // temporary buffer for sending data
volatile uint8_t tx_succesfull = 1;
static volatile uint8_t rxNE = 0;

void send_next(){
    //if(!tx_succesfull) return;
    static int lastdsz = 0;
    int buflen = RB_read(usbbuff);
    if(!buflen){
        if(lastdsz == 64) EP_Write(3, NULL, 0); // send ZLP after 64 bits packet when nothing more to send
        lastdsz = 0;
        return;
    }
    tx_succesfull = 0;
    EP_Write(3, (uint8_t*)usbbuff, buflen);
    lastdsz = buflen;
}

// put `buf` into queue to send
void USB_send(const char *buf){
    if(!buf || !usbON) return;
    int len = 0;
    const char *b = buf;
    while(*b++) ++len;
    if(!usbON || !len) return;
    int l = len;
    while(l){
        if(tx_succesfull) send_next();
        int a = RB_write(buf, l);
        l -= a;
        buf += a;
    }
}

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
            if(tx_succesfull) send_next();
    }
}

/**
 * @brief USB_receive
 * @param buf (i) - buffer[64] for received data
 * @return amount of received bytes
 */
uint8_t USB_receive(char *buf){
    if(!usbON || !rxNE) return 0;
    uint8_t sz = EP_Read(2, (uint16_t*)buf);
    uint16_t epstatus = KEEP_DTOG(USB->EPnR[2]);
    // keep stat_tx & set ACK rx
    USB->EPnR[2] = (epstatus & ~(USB_EPnR_STAT_TX)) ^ USB_EPnR_STAT_RX;
    rxNE = 0;
    return sz;
}
