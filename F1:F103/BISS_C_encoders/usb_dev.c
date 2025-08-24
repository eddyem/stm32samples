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
#include "strfunc.h" // for cmd interface
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

// It's good to use debug here ONLY to debug into USART!
// never try to debug USB into USB!!!
#undef DBG
#define DBG(x)
#undef DBGs
#define DBGs(x)

extern volatile uint32_t Tms;

// inbuf overflow when receiving
static volatile uint8_t bufovrfl[bTotNumEndpoints] = {0};

// receive buffer: hold data until chkin() call
static uint8_t volatile rcvbuf[bTotNumEndpoints][USB_RXBUFSZ];
static uint8_t volatile rcvbuflen[bTotNumEndpoints] = {0};
// line coding
#define DEFLC   {115200, 0, 0, 8}
static usb_LineCoding lineCoding[bTotNumEndpoints] = {DEFLC, DEFLC, DEFLC};
// CDC configured and ready to use
volatile uint8_t CDCready[bTotNumEndpoints] = {0};

// ring buffers for incoming and outgoing data
static uint8_t obuf[bTotNumEndpoints][RBOUTSZ], ibuf[bTotNumEndpoints][RBINSZ];
#define OBUF(N)  {.data = obuf[N], .length = RBOUTSZ, .head = 0, .tail = 0}
static volatile ringbuffer rbout[bTotNumEndpoints] = {OBUF(0), OBUF(1), OBUF(2)};
#define IBUF(N)  {.data = ibuf[N], .length = RBINSZ, .head = 0, .tail = 0}
static volatile ringbuffer rbin[bTotNumEndpoints] = {IBUF(0), IBUF(1), IBUF(2)};
// last send data size (<0 if USB transfer ready)
static volatile int lastdsz[bTotNumEndpoints] = {-1, -1, -1};

static void chkin(uint8_t ifno){
    if(bufovrfl[ifno]) return; // allow user to know that previous buffer was overflowed and cleared
    if(!rcvbuflen[ifno]) return;
    int w = RB_write((ringbuffer*)&rbin[ifno], (uint8_t*)rcvbuf[ifno], rcvbuflen[ifno]);
    if(w < 0){
        DBG("Can't write into buffer");
        return;
    }
    if(w != rcvbuflen[ifno]) bufovrfl[ifno] = 1;
    DBG("Put data into buffer");
    rcvbuflen[ifno] = 0;
    uint16_t status = KEEP_DTOG(USB->EPnR[1+ifno]); // don't change DTOG
    USB->EPnR[1+ifno] = (status & ~(USB_EPnR_STAT_TX|USB_EPnR_CTR_RX)) ^ USB_EPnR_STAT_RX; // prepare to get next data portion
}

// called from transmit EP to send next data portion or by user - when new transmission starts
static void send_next(uint8_t ifno){
    uint8_t usbbuff[USB_TXBUFSZ];
    int buflen = RB_read((ringbuffer*)&rbout[ifno], (uint8_t*)usbbuff, USB_TXBUFSZ);
    if(!CDCready[ifno]){
        lastdsz[ifno] = -1;
        return;
    }
    if(buflen == 0){
        if(lastdsz[ifno] == USB_TXBUFSZ){
            EP_Write(1+ifno, NULL, 0); // send ZLP after 64 bits packet when nothing more to send
            lastdsz[ifno] = 0;
        }else lastdsz[ifno] = -1; // OK. User can start sending data
        return;
    }else if(buflen < 0){
        DBG("Buff busy");
        lastdsz[ifno] = -1;
        return;
    }
    DBG("Got data in buf");
    DBGs(uhex2str(buflen));
    DBGs(uhex2str(ifno));
    EP_Write(1+ifno, (uint8_t*)usbbuff, buflen);
    lastdsz[ifno] = buflen;
}

// data IN/OUT handler
static void rxtx_handler(){
    uint8_t ifno = (USB->ISTR & USB_ISTR_EPID) - 1;
    DBG("rxtx_handler");
    DBGs(uhex2str(ifno));
    if(ifno > bTotNumEndpoints-1){
        DBG("wrong ifno");
        return;
    }
    uint16_t epstatus = KEEP_DTOG(USB->EPnR[1+ifno]);
    if(RX_FLAG(epstatus)){ // receive data
        DBG("Got data");
        if(rcvbuflen[ifno]){
            bufovrfl[ifno] = 1; // lost last data
            rcvbuflen[ifno] = 0;
            DBG("OVERFULL");
        }
        rcvbuflen[ifno] = EP_Read(1+ifno, (uint8_t*)rcvbuf[ifno]);
        DBGs(uhex2str(rcvbuflen[ifno]));
        USB->EPnR[1+ifno] = epstatus & ~(USB_EPnR_CTR_RX | USB_EPnR_STAT_RX | USB_EPnR_STAT_TX); // keep RX in STALL state until read data
        chkin(ifno); // try to write current data into RXbuf if it's not busy
    }else{ // tx successfull
        DBG("Tx OK");
        USB->EPnR[1+ifno] = (epstatus & ~(USB_EPnR_CTR_TX | USB_EPnR_STAT_TX)) ^ USB_EPnR_STAT_RX;
        send_next(ifno);
    }
}

// weak handlers: change them somewhere else if you want to setup USART
// SET_LINE_CODING
void WEAK linecoding_handler(uint8_t ifno, usb_LineCoding *lc){
    lineCoding[ifno] = *lc;
    DBG("linecoding_handler");
    DBGs(uhex2str(ifno));
}

static void clearbufs(uint8_t ifno){
    uint32_t T0 = Tms;
    while(Tms - T0 < 10){ // wait no more than 10ms
        if(1 == RB_clearbuf((ringbuffer*)&rbin[ifno])) break;
    }
    T0 = Tms;
    while(Tms - T0 < 10){
        if(1 == RB_clearbuf((ringbuffer*)&rbout[ifno])) break;
    }
}

// SET_CONTROL_LINE_STATE
void WEAK clstate_handler(uint8_t ifno, uint16_t val){
    DBG("clstate_handler");
    DBGs(uhex2str(ifno));
    DBGs(uhex2str(val));
    CDCready[ifno] = val; // CONTROL_DTR | CONTROL_RTS -> interface connected; 0 -> disconnected
    lastdsz[ifno] = -1;
    if(val) clearbufs(ifno);
}

// SEND_BREAK - disconnect interface and clear its buffers
void WEAK break_handler(uint8_t ifno){
    CDCready[ifno] = 0;
    DBG("break_handler()");
    DBGs(uhex2str(ifno));
}


// USB is configured: setup endpoints
void set_configuration(){
    DBG("set_configuration()");
    for(int i = 0; i < bTotNumEndpoints; ++i){
        IWDG->KR = IWDG_REFRESH;
        int r = EP_Init(1+i, EP_TYPE_BULK, USB_TXBUFSZ, USB_RXBUFSZ, rxtx_handler);
        if(r){
            DBG("Can't init EP");
            DBGs(uhex2str(i));
            DBGs(uhex2str(r));
        }
    }
}

// USB CDC CLASS request
void usb_class_request(config_pack_t *req, uint8_t *data, uint16_t datalen){
    uint8_t recipient = REQUEST_RECIPIENT(req->bmRequestType);
    uint8_t dev2host = (req->bmRequestType & 0x80) ? 1 : 0;
    uint8_t ifno = req->wIndex >> 1;
    if(ifno > bTotNumEndpoints-1 && ifno != 0xff){
        DBG("wrong ifno");
        return;
    }
    DBG("usb_class_request");
    DBGs(uhex2str(req->bRequest));
    switch(recipient){
        case REQ_RECIPIENT_INTERFACE:
            switch(req->bRequest){
                case SET_LINE_CODING:
                    DBG("SET_LINE_CODING");
                    if(!data || !datalen) break; // wait for data
                    if(datalen == sizeof(usb_LineCoding))
                        linecoding_handler(ifno, (usb_LineCoding*)data);
                break;
                case GET_LINE_CODING:
                    DBG("GET_LINE_CODING");
                    EP_WriteIRQ(0, (uint8_t*)&lineCoding[ifno], sizeof(lineCoding));
                break;
                case SET_CONTROL_LINE_STATE:
                    DBG("SET_CONTROL_LINE_STATE");
                    clstate_handler(ifno, req->wValue);
                break;
                case SEND_BREAK:
                    DBG("SEND_BREAK");
                    break_handler(ifno);
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
int USB_sendall(uint8_t ifno){
    uint32_t T0 = Tms;
    while(lastdsz[ifno] > 0){
        if(Tms - T0 > DISCONN_TMOUT){
            break_handler(ifno);
            return FALSE;
        }
        if(!CDCready[ifno]) return FALSE;
        IWDG->KR = IWDG_REFRESH;
    }
    return TRUE;
}

// put `buf` into queue to send
int USB_send(uint8_t ifno, const uint8_t *buf, int len){
    if(!buf || !CDCready[ifno] || !len) return FALSE;
    DBG("USB_send");
    while(len){
        if(!CDCready[ifno]) return FALSE;
        IWDG->KR = IWDG_REFRESH;
        int a = RB_write((ringbuffer*)&rbout[ifno], buf, len);
        if(a > 0){
            len -= a;
            buf += a;
        }else if(a == 0){ // overfull
            if(lastdsz[ifno] < 0) send_next(ifno);
        }
    }
    if(buf[len-1] == '\n' && lastdsz[ifno] < 0) send_next(ifno);
    return TRUE;
}

int USB_putbyte(uint8_t ifno, uint8_t byte){
    if(!CDCready[ifno]) return FALSE;
    int l = 0;
    while((l = RB_write((ringbuffer*)&rbout[ifno], &byte, 1)) != 1){
        if(!CDCready[ifno]) return FALSE;
        IWDG->KR = IWDG_REFRESH;
        if(l == 0){ // overfull
            if(lastdsz[ifno] < 0) send_next(ifno);
            continue;
        }
    }
    // send line if got EOL
    if(byte == '\n' && lastdsz[ifno] < 0) send_next(ifno);
    return TRUE;
}

int USB_sendstr(uint8_t ifno, const char *string){
    if(!string || !CDCready[ifno]) return FALSE;
    int len = strlen(string);
    if(!len) return FALSE;
    return USB_send(ifno, (const uint8_t*)string, len);
}

/**
 * @brief USB_receive - get binary data from receiving ring-buffer
 * @param buf (i) - buffer for received data
 * @param len - length of `buf`
 * @return amount of received bytes (negative, if overfull happened)
 */
int USB_receive(uint8_t ifno, uint8_t *buf, int len){
    chkin(ifno);
    if(bufovrfl[ifno]){
        DBG("Buffer overflow");
        DBGs(uhex2str(ifno));
        while(1 != RB_clearbuf((ringbuffer*)&rbin[ifno])); // run watchdog in case of problems
        bufovrfl[ifno] = 0;
        return -1;
    }
    int sz = RB_read((ringbuffer*)&rbin[ifno], buf, len);
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
int USB_receivestr(uint8_t ifno, char *buf, int len){
    chkin(ifno);
    if(bufovrfl[ifno]){
        while(1 != RB_clearbuf((ringbuffer*)&rbin[ifno]));
        bufovrfl[ifno] = 0;
        return -1;
    }
    int l = RB_readto((ringbuffer*)&rbin[ifno], '\n', (uint8_t*)buf, len);
    if(l < 1){
        if(rbin[ifno].length == RB_datalen((ringbuffer*)&rbin[ifno])){ // buffer is full but no '\n' found
            while(1 != RB_clearbuf((ringbuffer*)&rbin[ifno]));
            return -1;
        }
        return 0;
    }
    if(l == 0) return 0;
    buf[l-1] = 0; // replace '\n' with strend
    return l;
}

