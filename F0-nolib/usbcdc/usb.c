/*
 *                                                                                                  geany_encoding=koi8-r
 * usb.c
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
#include "usart.h"



static uint8_t buffer[BUFFSIZE+1];
static uint8_t len, rcvflag = 0;

// interrupt IN handler (never used?)
static uint16_t EP1_Handler(ep_t ep){
    if (ep.rx_flag){
        EP_Read(1, buffer);
        ep.status = SET_VALID_TX(ep.status);
        ep.status = KEEP_STAT_RX(ep.status);
    } else
    if (ep.tx_flag){
        ep.status = SET_VALID_RX(ep.status);
        ep.status = SET_STALL_TX(ep.status);
    }
    return ep.status;
}

// data IN/OUT handler
static uint16_t EP2_Handler(ep_t ep){
    if(ep.rx_flag){
        if(ep.rx_cnt > 0 && ep.rx_cnt < BUFFSIZE){
            rcvflag = 1;
            len = EP_Read(2, buffer);
            buffer[len] = 0;
            #ifdef EBUG
            MSG("read: ");
            if(len) SEND((char*)buffer);
            #endif
        }
        // end of transaction: clear DTOGs
        ep.status = CLEAR_DTOG_RX(ep.status);
        ep.status = CLEAR_DTOG_TX(ep.status);
        ep.status = SET_STALL_TX(ep.status);
    }else if (ep.tx_flag){
        ep.status = KEEP_STAT_TX(ep.status);
    }
    ep.status = SET_VALID_RX(ep.status);
    return ep.status;
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
    // allow RESET and CTRM interrupts
    USB -> CNTR = USB_CNTR_RESETM | USB_CNTR_CTRM;
    // clear flags
    USB -> ISTR = 0;
    // and activate pullup
    USB -> BCDR |= USB_BCDR_DPPU;
    NVIC_EnableIRQ(USB_IRQn);
}

void usb_proc(){
    static int8_t usbON = 0;
    if(USB_GetState() == USB_CONFIGURE_STATE){ // USB configured - activate other endpoints
        if(!usbON){ // endpoints not activated
            MSG("Configured; activate other endpoints\n");
            // make new BULK endpoint
            // Buffer have 1024 bytes, but last 256 we use for CAN bus
            // first free is 64; 768 - CAN data
            // free: 64   128   192   256   320   384   448   512   576   640   704
            // (first 192 free bytes are for EP0)
            EP_Init(1, EP_TYPE_INTERRUPT, 192, 192, EP1_Handler);
            EP_Init(2, EP_TYPE_BULK, 256, 256, EP2_Handler); // OUT - receive data
            EP_Init(3, EP_TYPE_BULK, 320, 320, EP2_Handler); // IN - transmit data
            usbON = 1;
        }else{
            if(rcvflag){
                /*
                 * don't process received data here: if it would come too fast you will loose a part
                 * It would be a good idea to collect incoming data in greater buffer and process it
                 * later (EX: echo "text" > /dev/ttyUSB1 will split into two writings!
                 */
                rcvflag = 0;
            }
            if(SETLINECODING()){
                SEND("got new linecoding");
                CLRLINECODING();
            }
        }
    }else{
        usbON = 0;
    }
}

void USB_send(char *buf){
    uint16_t l = 0;
    char *p = buf;
    while(*p++) ++l;
    EP_Write(3, (uint8_t*)buf, l);
}
