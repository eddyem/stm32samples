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
#ifdef EBUG
#include "usart.h"
#endif


static uint8_t buffer[64];
static uint8_t len, rcvflag = 0;

static uint16_t EP1_Handler(ep_t ep){
    MSG("EP1 ");
    if (ep.rx_flag){                            //Пришли новые данные
        MSG("read");
        EP_Read(1, buffer);
        EP_WriteIRQ(1, buffer, ep.rx_cnt);
        ep.status = SET_VALID_TX(ep.status);    //TX
        ep.status = KEEP_STAT_RX(ep.status);    //RX оставляем в NAK
    } else
    if (ep.tx_flag){                            //Данные успешно переданы
        MSG("write");
        ep.status = SET_VALID_RX(ep.status);    //RX в VALID
        ep.status = SET_STALL_TX(ep.status);    //TX в STALL
    }
    MSG("; end\n");
    return ep.status;
}

// write handler
static uint16_t EP2_Handler(ep_t ep){
    MSG("EP2 ");
    if (ep.rx_flag){                            //Пришли новые данные
        MSG("read");
        #ifdef EBUG
        printu(ep.rx_cnt);
        #endif
        if(ep.rx_cnt > 2){
            len = ep.rx_cnt;
            rcvflag = 1;
            EP_Read(2, buffer);
        }
        //EP_WriteIRQ(3, buffer, ep.rx_cnt);
        //Так как мы ожидаем новый запрос от хоста, устанавливаем
        ep.status = SET_VALID_RX(ep.status);
        //ep.status = SET_STALL_TX(ep.status);
        //ep.status = SET_VALID_TX(ep.status);    //TX
        //ep.status = KEEP_STAT_RX(ep.status);    //RX оставляем в NAK
    } else
    if (ep.tx_flag){                            //Данные успешно переданы
        MSG("write");
        ep.status = SET_VALID_RX(ep.status);    //RX в VALID
        ep.status = SET_STALL_TX(ep.status);    //TX в STALL
    }
    MSG("; end\n");
    return ep.status;
}

// read handler
static uint16_t EP3_Handler(ep_t ep){
    MSG("EP3 ");
    if (ep.rx_flag){                            //Пришли новые данные
        MSG("read");
        EP_Read(3, buffer);
        ep.status = SET_VALID_TX(ep.status);    //TX
        ep.status = KEEP_STAT_RX(ep.status);    //RX оставляем в NAK
    } else
    if (ep.tx_flag){                            //Данные успешно переданы
        MSG("write");
        ep.status = SET_VALID_RX(ep.status);    //RX в VALID
        ep.status = SET_STALL_TX(ep.status);    //TX в STALL
    }
    MSG("; end\n");
    return ep.status;
}

void USB_setup(){
    RCC->APB1ENR |= RCC_APB1ENR_CRSEN | RCC_APB1ENR_USBEN; // enable CRS (hsi48 sync) & USB
    RCC->CFGR3 &= ~RCC_CFGR3_USBSW; // reset USB
    RCC->CR2 |= RCC_CR2_HSI48ON; // turn ON HSI48
    while(!(RCC->CR2 & RCC_CR2_HSI48RDY));
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;
    CRS->CFGR &= ~CRS_CFGR_SYNCSRC;
    CRS->CFGR |= CRS_CFGR_SYNCSRC_1; // USB SOF selected as sync source
    CRS->CR |= CRS_CR_AUTOTRIMEN; // enable auto trim
    CRS->CR |= CRS_CR_CEN; // enable freq counter & block CRS->CFGR as read-only
    RCC->CFGR |= RCC_CFGR_SW;
    //Разрешаем прерывания по RESET и CTRM
    USB -> CNTR = USB_CNTR_RESETM | USB_CNTR_CTRM;
    //Сбрасываем флаги
    USB -> ISTR = 0;
    //Включаем подтяжку на D+
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
            EP_Init(1, EP_TYPE_INTERRUPT, 256, 320, EP1_Handler);
            EP_Init(2, EP_TYPE_BULK, 384, 448, EP2_Handler);
            EP_Init(3, EP_TYPE_BULK, 512, 576, EP3_Handler);
            usbON = 1;
        }else{
            if(rcvflag){
                EP_Write(3, buffer, len);
                MSG("read: ");
                if(len > 2) while(LINE_BUSY == usart_send((char*)&buffer[2], len-2));
                rcvflag = 0;
            }
        }
    }else{
        usbON = 0;
    }
}
