/*
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

#include <stm32f1.h>
#include <string.h>

#include "ringbuffer.h"
#include "usart.h"
#include "usb_descr.h"
#include "usb_dev.h"

// Class-Specific Control Requests
#define SEND_ENCAPSULATED_COMMAND       0x00    // unused
#define GET_ENCAPSULATED_RESPONSE       0x01    // unused
#define SET_COMM_FEATURE                0x02    // unused
#define GET_COMM_FEATURE                0x03    // unused
#define CLEAR_COMM_FEATURE              0x04    // unused
#define SET_LINE_CODING                 0x20
#define GET_LINE_CODING                 0x21
#define SET_CONTROL_LINE_STATE          0x22
#define SEND_BREAK                      0x23

// control line states
#define CONTROL_DTR                     0x01
#define CONTROL_RTS                     0x02

// inbuf overflow when receiving
static volatile uint8_t bufovrfl = 0;

// receive buffer: hold data until chkin() call
static uint8_t volatile rcvbuf[USB_RXBUFSZ];
static uint8_t volatile rcvbuflen = 0;
// line coding
usb_LineCoding WEAK lineCoding = {115200, 0, 0, 8};
// CDC configured and ready to use
volatile uint8_t CDCready = 0;

// ring buffers for incoming and outgoing data
static uint8_t obuf[RBOUTSZ], ibuf[RBINSZ];
static volatile ringbuffer rbout = {.data = obuf, .length = RBOUTSZ, .head = 0, .tail = 0};
static volatile ringbuffer rbin = {.data = ibuf, .length = RBINSZ, .head = 0, .tail = 0};
// last send data size
static volatile int lastdsz = 0;

static void chkin(){
    if(bufovrfl) return; // allow user to know that previous buffer was overflowed and cleared
    if(!rcvbuflen) return;
    int w = RB_write((ringbuffer*)&rbin, (uint8_t*)rcvbuf, rcvbuflen);
    if(w < 0){
        DBG("Can't write into buffer");
        return;
    }
    if(w != rcvbuflen) bufovrfl = 1;
    DBG("Put data into buffer");
    rcvbuflen = 0;
    uint16_t status = KEEP_DTOG(USB->EPnR[1]); // don't change DTOG
    USB->EPnR[1] = (status & ~(USB_EPnR_STAT_TX|USB_EPnR_CTR_RX)) ^ USB_EPnR_STAT_RX; // prepare to get next data portion
}

// called from transmit EP to send next data portion or by user - when new transmission starts
static void send_next(){
    uint8_t usbbuff[USB_TXBUFSZ];
    int buflen = RB_read((ringbuffer*)&rbout, (uint8_t*)usbbuff, USB_TXBUFSZ);
    if(!CDCready){
        lastdsz = -1;
        return;
    }
    if(buflen == 0){
        if(lastdsz == USB_TXBUFSZ){
            EP_Write(1, NULL, 0); // send ZLP after 64 bits packet when nothing more to send
            lastdsz = 0;
        }else lastdsz = -1;
        return;
    }else if(buflen < 0){
        DBG("Buff busy");
        lastdsz = -1;
        return;
    }
    DBG("Got data in buf");
    DBGs(uhex2str(buflen));
    EP_Write(1, (uint8_t*)usbbuff, buflen);
    lastdsz = buflen;
}

// data IN/OUT handler
static void rxtx_handler(){
    uint16_t epstatus = KEEP_DTOG(USB->EPnR[1]);
    if(RX_FLAG(epstatus)){ // receive data
        DBG("Got data");
        if(rcvbuflen){
            bufovrfl = 1; // lost last data
            rcvbuflen = 0;
        }
        rcvbuflen = EP_Read(1, (uint8_t*)rcvbuf);
        USB->EPnR[1] = epstatus & ~(USB_EPnR_CTR_RX | USB_EPnR_STAT_RX | USB_EPnR_STAT_TX); // keep RX in STALL state until read data
        chkin(); // try to write current data into RXbuf if it's not busy
    }else{ // tx successfull
        DBG("Tx OK");
        USB->EPnR[1] = (epstatus & ~(USB_EPnR_CTR_TX | USB_EPnR_STAT_TX)) ^ USB_EPnR_STAT_RX;
        send_next();
    }
}

// weak handlers: change them somewhere else if you want to setup USART
// SET_LINE_CODING
void WEAK linecoding_handler(usb_LineCoding *lc){
    lineCoding = *lc;
    DBG("linecoding_handler");
}

// SET_CONTROL_LINE_STATE
void WEAK clstate_handler(uint16_t val){
    DBG("clstate_handler");
    DBGs(uhex2str(val));
    CDCready = val; // CONTROL_DTR | CONTROL_RTS -> interface connected; 0 -> disconnected
}

// SEND_BREAK
void WEAK break_handler(){
    CDCready = 0;
    DBG("break_handler()");
}


// USB is configured: setup endpoints
void set_configuration(){
    DBG("set_configuration()");
    EP_Init(1, EP_TYPE_BULK, USB_TXBUFSZ, USB_RXBUFSZ, rxtx_handler); // IN1 and OUT1
}

// PL2303 CLASS request
void usb_class_request(config_pack_t *req, uint8_t *data, uint16_t datalen){
    uint8_t recipient = REQUEST_RECIPIENT(req->bmRequestType);
    uint8_t dev2host = (req->bmRequestType & 0x80) ? 1 : 0;
    DBG("usb_class_request");
    DBGs(uhex2str(req->bRequest));
    switch(recipient){
        case REQ_RECIPIENT_INTERFACE:
            switch(req->bRequest){
                case SET_LINE_CODING:
                    DBG("SET_LINE_CODING");
                    if(!data || !datalen) break; // wait for data
                    if(datalen == sizeof(usb_LineCoding))
                        linecoding_handler((usb_LineCoding*)data);
                break;
                case GET_LINE_CODING:
                    DBG("GET_LINE_CODING");
                    EP_WriteIRQ(0, (uint8_t*)&lineCoding, sizeof(lineCoding));
                break;
                case SET_CONTROL_LINE_STATE:
                    DBG("SET_CONTROL_LINE_STATE");
                    clstate_handler(req->wValue);
                break;
                case SEND_BREAK:
                    DBG("SEND_BREAK");
                    break_handler();
                break;
                default:
                    DBG("Wrong");
                    DBGs(uhex2str(req->bRequest));
                    DBGs(uhex2str(datalen));
            }
        break;
        default:
            DBG("Wrong");
            DBGs(uhex2str(recipient));
            DBGs(uhex2str(datalen));
            DBGs(uhex2str(req->bRequest));
            if(dev2host) EP_WriteIRQ(0, NULL, 0);
    }
    if(!dev2host) EP_WriteIRQ(0, NULL, 0);
}

// blocking send full content of ring buffer
int USB_sendall(){
    while(lastdsz > 0){
        if(!CDCready) return FALSE;
        IWDG->KR = IWDG_REFRESH;
    }
    return TRUE;
}

// put `buf` into queue to send
int USB_send(const uint8_t *buf, int len){
    if(!buf || !CDCready || !len) return FALSE;
    DBG("USB_send");
    while(len){
        if(!CDCready) return FALSE;
        IWDG->KR = IWDG_REFRESH;
        int a = RB_write((ringbuffer*)&rbout, buf, len);
        if(a > 0){
            len -= a;
            buf += a;
        }else if(a == 0){ // overfull
            if(lastdsz < 0) send_next();
        }
    }
    if(buf[len-1] == '\n' && lastdsz < 0) send_next();
    return TRUE;
}

int USB_putbyte(uint8_t byte){
    if(!CDCready) return FALSE;
    int l = 0;
    while((l = RB_write((ringbuffer*)&rbout, &byte, 1)) != 1){
        if(!CDCready) return FALSE;
        IWDG->KR = IWDG_REFRESH;
        if(l == 0){ // overfull
            if(lastdsz < 0) send_next();
            continue;
        }
    }
    if(byte == '\n' && lastdsz < 0) send_next();
    return TRUE;
}

int USB_sendstr(const char *string){
    if(!string || !CDCready) return FALSE;
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
        DBG("Buffer overflow");
        while(1 != RB_clearbuf((ringbuffer*)&rbin));
        bufovrfl = 0;
        return -1;
    }
    int sz = RB_read((ringbuffer*)&rbin, buf, len);
    if(sz < 0) return 0; // buffer in writting state
    DBG("usb read");
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
    if(l == 0) return 0;
    buf[l-1] = 0; // replace '\n' with strend
    return l;
}

