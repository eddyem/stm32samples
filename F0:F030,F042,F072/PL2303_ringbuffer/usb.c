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

// USB read/write for text or binary data through ring-buffers (mean 5187476b/s)

#include "ringbuffer.h"
#include "usb.h"
#include "usb_lib.h"

static uint8_t usbbuff[USB_TXBUFSZ]; // temporary buffer for sending data
// ring buffers for incoming and outgoing data
static uint8_t obuf[RBOUTSZ], ibuf[RBINSZ];
static ringbuffer out = {.data = obuf, .length = RBOUTSZ, .head = 0, .tail = 0};
static ringbuffer in = {.data = ibuf, .length = RBINSZ, .head = 0, .tail = 0};
// transmission is succesfull
static volatile uint8_t tx_succesfull = 1;
static volatile uint8_t bufovrfl = 0;

static void send_next(){
    if(!tx_succesfull) return;
    static int lastdsz = 0;
    int buflen = RB_read(&out, usbbuff, USB_TXBUFSZ);
    if(!buflen){
        if(lastdsz == 64) EP_Write(3, NULL, 0); // send ZLP after 64 bits packet when nothing more to send
        lastdsz = 0;
        return;
    }
    tx_succesfull = 0;
    EP_Write(3, usbbuff, buflen);
    lastdsz = buflen;
}

// send full content of ring buffer
int USB_sendall(){
    while(RB_datalen(&out)){
        send_next();
        if(!usbON) return 0;
    }
    return 1;
}

// put `buf` into queue to send
int USB_send(const uint8_t *buf, int len){
    if(!buf || !usbON || !len) return 0;
    while(len){
        if(tx_succesfull) send_next();
        int a = RB_write(&out, buf, len);
        len -= a;
        buf += a;
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
 * @param buf (i) - buffer[64] for received data
 * @return amount of received bytes (negative, if overfull happened)
 */
int USB_receive(uint8_t *buf, int len){
    int sz = RB_read(&in, buf, len);
    if(bufovrfl){
        RB_clearbuf(&in);
        sz = -sz;
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
    int l = RB_readto(&in, '\n', (uint8_t*)buf, len);
    if(l < 0 || bufovrfl) RB_clearbuf(&in);
    else buf[l] = 0; // replace '\n' with strend
    if(bufovrfl){
        if(l > 0) l = -l;
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
    tx_succesfull = 1;
    uint16_t epstatus = KEEP_DTOG_STAT(USB->EPnR[3]);
    // clear CTR keep DTOGs & STATs
    USB->EPnR[3] = (epstatus & ~(USB_EPnR_CTR_TX)); // clear TX ctr
}

static void receive_Handler(){ // EP2OUT
    uint16_t epstatus = KEEP_DTOG(USB->EPnR[2]);
    uint8_t sz = endpoints[2].rx_cnt;
    if(sz){
        if(RB_write(&in, endpoints[2].rx_buf, sz) != sz) bufovrfl = 1;
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
            if(tx_succesfull) send_next();
    }
}
