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
#include "flash.h"
#include "usb.h"
#include "usb_lib.h"
#include "usart.h"

// incoming buffer size
#define IDATASZ     (256)
static uint8_t incoming_data[IDATASZ];
static uint8_t ovfl = 0;
static uint16_t idatalen = 0;
static volatile uint8_t tx_succesfull = 0;
static int8_t usbON = 0; // ==1 when USB fully configured

// interrupt IN handler (never used?)
static uint16_t EP1_Handler(ep_t ep){
    if (ep.rx_flag){
        ep.status = SET_VALID_TX(ep.status);
        ep.status = KEEP_STAT_RX(ep.status);
    }else if (ep.tx_flag){
        ep.status = SET_VALID_RX(ep.status);
        ep.status = SET_STALL_TX(ep.status);
    }
    return ep.status;
}

// data IN/OUT handler
static uint16_t EP23_Handler(ep_t ep){
    if(ep.rx_flag){
        int rd = ep.rx_cnt, rest = IDATASZ - idatalen;
        if(rd){
            if(rd <= rest){
                idatalen += EP_Read(2, (uint16_t*)&incoming_data[idatalen]);
                ovfl = 0;
            }else{
                ep.status = SET_NAK_RX(ep.status);
                ovfl = 1;
                return ep.status;
            }
        }
        // end of transaction: clear DTOGs
        ep.status = CLEAR_DTOG_RX(ep.status);
        ep.status = CLEAR_DTOG_TX(ep.status);
        ep.status = SET_STALL_TX(ep.status);
    }else if (ep.tx_flag){
        ep.status = KEEP_STAT_TX(ep.status);
        tx_succesfull = 1;
    }
    ep.status = SET_VALID_RX(ep.status);
    return ep.status;
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
    NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn );
}

void usb_proc(){
    if(USB_GetState() == USB_CONFIGURE_STATE){ // USB configured - activate other endpoints
        if(!usbON){ // endpoints not activated
            // make new BULK endpoint
            // Buffer have 1024 bytes, but last 256 we use for CAN bus (30.2 of RM: USB main features)
            EP_Init(1, EP_TYPE_INTERRUPT, 10, 0, EP1_Handler); // IN1 - transmit
            EP_Init(2, EP_TYPE_BULK, 0, USB_RXBUFSZ, EP23_Handler); // OUT2 - receive data
            EP_Init(3, EP_TYPE_BULK, USB_TXBUFSZ, 0, EP23_Handler); // IN3 - transmit data
            usbON = 1;
        }
    }else{
        usbON = 0;
    }
}

extern uint8_t USB_connected;
void USB_send(const char *buf){
    if(!USB_configured()){
        DBG("USB not configured");
        return;
    }
    if(!USB_connected) return; // no connection -> no need to send data into nothing
    char tmpbuf[USB_TXBUFSZ];
    uint16_t l = 0, ctr = 0;
    const char *p = buf;
    while(*p++) ++l;
    while(l){
        uint16_t proc = 0, s  = (l > USB_TXBUFSZ - 1) ? USB_TXBUFSZ - 1: l;
        for(int i = 0; i < s; ++i, ++proc){
            char c = buf[ctr+proc];
            if(c == '\n' && the_conf.defflags & FLAG_STRENDRN){ // add '\r' before '\n'
                tmpbuf[i++] = '\r';
                if(i == s) ++s;
            }
            if(c == 0x1B) tmpbuf[i] = 'E'; // ESC
            else if(c == 0x7F) tmpbuf[i] = 'B'; // Backspace
            else tmpbuf[i] = c;
        }
        tx_succesfull = 0;
        EP_Write(3, (uint8_t*)tmpbuf, s);
        uint32_t ctra = 1000000;
        while(--ctra && tx_succesfull == 0);
        l -= proc;
        ctr += proc;
    }
}

/**
 * @brief USB_receive
 * @param buf (i) - buffer for received data
 * @param bufsize - its size
 * @return amount of received bytes
 */
int USB_receive(char *buf, int bufsize){
    if(!bufsize || !idatalen) return 0;
    USB->CNTR = 0;
    int sz = (idatalen > bufsize) ? bufsize : idatalen, rest = idatalen - sz;
    for(int i = 0; i < sz; ++i) buf[i] = incoming_data[i];
    if(rest > 0){
        uint8_t *ptr = &incoming_data[sz];
        for(int i = 0; i < rest; ++i) incoming_data[i] = *ptr++;
        //memmove(incoming_data, &incoming_data[sz], rest); - hardfault on memcpy&memmove
        idatalen = rest;
    }else idatalen = 0;
    if(ovfl){
        EP23_Handler(endpoints[2]);
        uint16_t epstatus = USB->EPnR[2];
        epstatus = CLEAR_DTOG_RX(epstatus);
        epstatus = SET_VALID_RX(epstatus);
        USB->EPnR[2] = epstatus;
    }
    USB->CNTR = USB_CNTR_RESETM | USB_CNTR_CTRM;
    return sz;
}

/**
 * @brief USB_configured
 * @return 1 if USB is in configured state
 */
int USB_configured(){
    return usbON;
}


/*
 * default handlers
 *
// SET_LINE_CODING
void WEAK linecoding_handler(usb_LineCoding __attribute__((unused)) *lc){
    DBG("WEAK LH");
}

// SET_CONTROL_LINE_STATE
void WEAK clstate_handler(uint16_t __attribute__((unused)) val){
    DBG("WEAK CLSH");
}

// SEND_BREAK
void WEAK break_handler(){
    DBG("WEAK BH");
}*/

// handler of vendor requests
void WEAK vendor_handler(config_pack_t *packet){
    if(packet->bmRequestType & 0x80){ // read
        uint8_t c;
        switch(packet->wValue){
            case 0x8484:
                c = 2;
            break;
            case 0x0080:
                c = 1;
            break;
            case 0x8686:
                c = 0xaa;
            break;
            default:
                c = 0;
        }
        EP_WriteIRQ(0, &c, 1);
    }else{ // write ZLP
        EP_WriteIRQ(0, (uint8_t *)0, 0);
    }
}
