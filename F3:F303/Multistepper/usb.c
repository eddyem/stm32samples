/*
 * This file is part of the multistepper project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#ifdef EBUG
#include "strfunc.h"
#endif

static volatile uint8_t usbbuff[USB_TXBUFSZ]; // temporary buffer for sending data
// ring buffers for incoming and outgoing data
static uint8_t obuf[RBOUTSZ], ibuf[RBINSZ];
volatile ringbuffer rbout = {.data = obuf, .length = RBOUTSZ, .head = 0, .tail = 0};
volatile ringbuffer rbin = {.data = ibuf, .length = RBINSZ, .head = 0, .tail = 0};
// inbuf overflow when receiving
volatile uint8_t bufovrfl = 0;
// last send data size
static volatile int lastdsz = 0;

// called from transmit EP
void send_next(){
    int buflen = RB_read((ringbuffer*)&rbout, (uint8_t*)usbbuff, USB_TXBUFSZ);
    if(buflen == 0){
        if(lastdsz == 64) EP_Write(3, NULL, 0); // send ZLP after 64 bits packet when nothing more to send
        lastdsz = 0;
        return;
    }else if(buflen < 0){
        EP_Write(3, NULL, 0); // send ZLP if buffer is in writting state now
        return;
    }
    EP_Write(3, (uint8_t*)usbbuff, buflen);
    lastdsz = buflen;
}

// blocking send full content of ring buffer
int USB_sendall(){
    while(lastdsz > 0){
        if(!usbON) return FALSE;
    }
    return TRUE;
}

// put `buf` into queue to send
int USB_send(const uint8_t *buf, int len){
    if(!buf || !usbON || !len) return FALSE;
    while(len){
        int a = RB_write((ringbuffer*)&rbout, buf, len);
        if(a > 0){
            len -= a;
            buf += a;
        } else if (a < 0) continue; // do nothing if buffer is in reading state
        if(lastdsz == 0) send_next(); // need to run manually - all data sent, so no IRQ on IN
    }
    return TRUE;
}

int USB_putbyte(uint8_t byte){
    if(!usbON) return FALSE;
    int l = 0;
    while((l = RB_write((ringbuffer*)&rbout, &byte, 1)) != 1){
        if(l < 0) continue;
    }
    if(lastdsz == 0) send_next(); // need to run manually - all data sent, so no IRQ on IN
    return TRUE;
}

int USB_sendstr(const char *string){
    if(!string || !usbON) return FALSE;
    int len = 0;
    const char *b = string;
    while(*b++) ++len;
    if(!len) return FALSE;
    return USB_send((const uint8_t*)string, len);
}

/**
 * @brief USB_receive - get binary data from receiving ring-buffer
 * @param buf (i) - buffer for received data
 * @param len - length of `buf`
 * @return amount of received bytes (negative, if overfull happened)
 */
int USB_receive(uint8_t *buf, int len){
    chkin();
    if(bufovrfl){
        while(1 != RB_clearbuf((ringbuffer*)&rbin));
        bufovrfl = 0;
        return -1;
    }
    int sz = RB_read((ringbuffer*)&rbin, buf, len);
    if(sz < 0) return 0; // buffer in writting state
    return sz;
}

/**
 * @brief USB_receivestr - get string up to '\n' and replace '\n' with 0
 * @param buf - receiving buffer
 * @param len - its length
 * @return strlen or negative value indicating overflow (if so, string won't be ends with 0 and buffer should be cleared)
 */
int USB_receivestr(char *buf, int len){
    chkin();
    if(bufovrfl){
        while(1 != RB_clearbuf((ringbuffer*)&rbin));
        bufovrfl = 0;
        return -1;
    }
    int l = RB_readto((ringbuffer*)&rbin, '\n', (uint8_t*)buf, len);
    if(l < 1){
        if(rbin.length == RB_datalen((ringbuffer*)&rbin)){ // buffer is full but no '\n' found
            while(1 != RB_clearbuf((ringbuffer*)&rbin));
            return -1;
        }
        return 0;
    }
#ifdef EBUG
    USB_sendstr("readto, l="); USB_sendstr(u2str(l)); newline();
#endif
    if(l == 0) return 0;
    buf[l-1] = 0; // replace '\n' with strend
    return l;
}

