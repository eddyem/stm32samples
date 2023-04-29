/*
 * This file is part of the SevenCDCs project.
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

#include "stm32f3.h"
#include "debug.h"
#include "hardware.h"
#include "strfunc.h"
#include "usart.h"
#include "usb.h"
#include <string.h>

static volatile int idatalen = 0; // received data line length (including '\n')
// USARTs registers
static volatile USART_TypeDef *USARTx[USARTSNO+1] = {0, USART1, USART2, USART3};

volatile int linerdy = 0,   // received data ready
    dlen    = 0,            // length of data (including '\n') in current buffer
    bufovr  = 0;            // input buffer overfull
// USARTs speeds
static usb_LineCoding lineCodings[USARTSNO+1] = {
    {9600, USB_CDC_1_STOP_BITS, USB_CDC_NO_PARITY, 8}, {0}};

usb_LineCoding *getLineCoding(int ifNo){
    int usartNo = ifNo - USART1_EPNO + 1;
    if(usartNo < 1 || usartNo > USARTSNO) return lineCodings;
    return &lineCodings[usartNo];
// TODO: fixme for different settings
    //lineCoding.dwDTERate = speeds[usartNo];
    //lineCoding.bCharFormat = USB_CDC_1_STOP_BITS;
    //lineCoding.bParityType = USB_CDC_NO_PARITY;
    //lineCoding.bDataBits = 8;
}

static void usart_putchar(int no, uint8_t ch){
    while(!(USARTx[no]->ISR & USART_ISR_TXE));
    USARTx[no]->TDR = ch;
}
void usart_sendn(int no, const uint8_t *str, int L){
    if(!str || L < 0 || no < 1 || no > USARTSNO) return;
    for(int i = 0; i < L; ++i){
        usart_putchar(no, str[i]);
    }
}

// setup all USARTs
void usarts_setup(){
    // clock
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_USART3EN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    for(int i = USART1_IDX; i <= USART3_IDX; ++i)
        usart_config(i, lineCodings);
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_EnableIRQ(USART3_IRQn);
}

void usart_config(uint8_t ifNo, usb_LineCoding *lc){
    int usartNo = ifNo - USART1_IDX + 1;
    if(usartNo < 1 || usartNo > USARTSNO) return;
    DBGmesg("setup USART"); DBGmesg(u2str(usartNo)); DBGnl();
    lineCodings[usartNo] = *lc;
    // FIXME: change also parity and so on
    volatile USART_TypeDef *U = USARTx[usartNo];
    U->CR1 = 0; // disable for reconfigure
    U->ICR = 0xffffffff; // clear all flags
    U->BRR = SysFreq / lc->dwDTERate;
    U->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_RXNEIE; // 1start,8data,nstop; enable Rx,Tx,USART
    uint32_t tmout = 16000000;
    while(!(U->ISR & USART_ISR_TC)){if(--tmout == 0) break;} // polling idle frame Transmission
    U->ICR = 0xffffffff; // clear all flags again
}

void usart1_exti25_isr(){
    if(USART1->ISR & USART_ISR_RXNE){ // RX not emty - receive next char
        DBG("got\n");
        // read RDR clears flag
        uint8_t rb = USART1->RDR;
        USB_putbyte(USART1_IDX, rb);
    }
}

void usart2_exti26_isr(){
    if(USART2->ISR & USART_ISR_RXNE){
        DBG("got\n");
        uint8_t rb = USART2->RDR;
        USB_putbyte(USART2_IDX, rb);
    }
}

void usart3_exti28_isr(){
    if(USART3->ISR & USART_ISR_RXNE){
        DBG("got\n");
        uint8_t rb = USART3->RDR;
        USB_putbyte(USART3_IDX, rb);
    }
}
