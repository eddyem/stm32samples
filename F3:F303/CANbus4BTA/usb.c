/*
 * This file is part of the canbus4bta project.
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
#include "usb.h"
#include "usb_lib.h"

static volatile uint8_t usbbuff[USB_TXBUFSZ]; // temporary buffer for sending data
// ring buffers for incoming and outgoing data
static uint8_t obuf[RBOUTSZ], ibuf[RBINSZ];
volatile ringbuffer rbout = {.data = obuf, .length = RBOUTSZ, .head = 0, .tail = 0};
volatile ringbuffer rbin = {.data = ibuf, .length = RBINSZ, .head = 0, .tail = 0};
// transmission is succesfull
volatile uint8_t bufisempty = 1;
volatile uint8_t bufovrfl = 0;

void send_next(){
    if(bufisempty) return;
    static int lastdsz = 0;
    int buflen = RB_read((ringbuffer*)&rbout, (uint8_t*)usbbuff, USB_TXBUFSZ);
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
        int a = RB_write((ringbuffer*)&rbout, buf, len);
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
    while(0 == RB_write((ringbuffer*)&rbout, &byte, 1)){
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
    int sz = RB_read((ringbuffer*)&rbin, buf, len);
    if(bufovrfl){
        RB_clearbuf((ringbuffer*)&rbin);
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
    int l = RB_readto((ringbuffer*)&rbin, '\n', (uint8_t*)buf, len);
    if(l == 0) return 0;
    if(--l < 0 || bufovrfl) RB_clearbuf((ringbuffer*)&rbin);
    else buf[l] = 0; // replace '\n' with strend
    if(bufovrfl){
        if(l > 0) l = -l;
        else l = -1;
        bufovrfl = 0;
    }
    return l;
}

