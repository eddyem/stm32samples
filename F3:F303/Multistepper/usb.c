/*
 * This file is part of the multistepper project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <string.h>

#include "hardware.h"
#include "ringbuffer.h"
#include "usb.h"
#include "usb_lib.h"

static volatile uint8_t usbbuff[USB_TXBUFSZ]; // temporary buffer for sending data
// ring buffers for incoming and outgoing data
static uint8_t obuf[RBOUTSZ], ibuf[RBINSZ];
static volatile ringbuffer out = {.data = obuf, .length = RBOUTSZ, .head = 0, .tail = 0};
static volatile ringbuffer in = {.data = ibuf, .length = RBINSZ, .head = 0, .tail = 0};
// transmission is succesfull
static volatile uint8_t bufisempty = 1;
static volatile uint8_t bufovrfl = 0;

static void send_next(){
    if(bufisempty) return;
    static int lastdsz = 0;
    int buflen = RB_read((ringbuffer*)&out, (uint8_t*)usbbuff, USB_TXBUFSZ);
    if(!buflen){
        if(lastdsz == 64) EP_Write(3, NULL, 0); // send ZLP after 64 bits packet when nothing more to send
        lastdsz = 0;
        bufisempty = 1;
        return;
    }
    EP_Write(3, (uint8_t*)usbbuff, buflen);
    lastdsz = buflen;
}

// blocking send full content of ring buffer
int USB_sendall(){
    while(!bufisempty){
        if(!usbON) return 0;
    }
    return 1;
}

// put `buf` into queue to send
int USB_send(const uint8_t *buf, int len){
    if(!buf || !usbON || !len) return 0;
    while(len){
        int a = RB_write((ringbuffer*)&out, buf, len);
        len -= a;
        buf += a;
        if(bufisempty){
            bufisempty = 0;
            send_next();
        }
    }
    return 1;
}

int USB_putbyte(uint8_t byte){
    if(!usbON) return 0;
    while(0 == RB_write((ringbuffer*)&out, &byte, 1)){
        if(bufisempty){
            bufisempty = 0;
            send_next();
        }
    }
    return 1;
}

int USB_sendstr(const char *string){
    if(!string || !usbON) return 0;
    int len = 0;
    const char *b = string;
    while(*b++) ++len;
    if(!len) return 0;
    return USB_send((const uint8_t*)string, len);
}

/**
 * @brief USB_receive - get binary data from receiving ring-buffer
 * @param buf (i) - buffer for received data
 * @param len - length of `buf`
 * @return amount of received bytes (negative, if overfull happened)
 */
int USB_receive(uint8_t *buf, int len){
    int sz = RB_read((ringbuffer*)&in, buf, len);
    if(bufovrfl){
        RB_clearbuf((ringbuffer*)&in);
        if(!sz) sz = -1;
        else sz = -sz;
        bufovrfl = 0;
    }
    return sz;
}

/**
 * @brief USB_receivestr - get string up to '\n' and replace '\n' with 0
 * @param buf - receiving buffer
 * @param len - its length
 * @return strlen or negative value indicating overflow (if so, string won't be ends with 0 and buffer should be cleared)
 */
int USB_receivestr(char *buf, int len){
    int l = RB_readto((ringbuffer*)&in, '\n', (uint8_t*)buf, len);
    if(l == 0) return 0;
    if(--l < 0 || bufovrfl) RB_clearbuf((ringbuffer*)&in);
    else buf[l] = 0; // replace '\n' with strend
    if(bufovrfl){
        if(l > 0) l = -l;
        else l = -1;
        bufovrfl = 0;
    }
    return l;
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
    uint16_t epstatus = KEEP_DTOG_STAT(USB->EPnR[3]);
    // clear CTR keep DTOGs & STATs
    USB->EPnR[3] = (epstatus & ~(USB_EPnR_CTR_TX)); // clear TX ctr
    send_next();
}

static void receive_Handler(){ // EP2OUT
    uint8_t buf[USB_RXBUFSZ];
    uint16_t epstatus = KEEP_DTOG(USB->EPnR[2]);
    uint8_t sz = EP_Read(2, (uint16_t*)buf);
    if(sz){
        if(RB_write((ringbuffer*)&in, buf, sz) != sz) bufovrfl = 1;
    }
    // keep stat_tx & set ACK rx, clear RX ctr
    USB->EPnR[2] = (epstatus & ~USB_EPnR_CTR_RX) ^ USB_EPnR_STAT_RX;
}

void USB_proc(){
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
    }
}
