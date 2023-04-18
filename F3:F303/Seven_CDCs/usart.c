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
#include "hardware.h"
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
static int speeds[USARTSNO+1] = {0};

usb_LineCoding *getLineCoding(int usartNo){
    static usb_LineCoding lineCoding; // `static` - to return pointer to it
    if(usartNo < 1 || usartNo > USARTSNO) return NULL;
// TODO: fixme
    lineCoding.dwDTERate = speeds[usartNo];
    lineCoding.bCharFormat = USB_CDC_1_STOP_BITS;
    lineCoding.bParityType = USB_CDC_NO_PARITY;
    lineCoding.bDataBits = 8;
    return &lineCoding;
}

void usart_putchar(uint8_t ch){
    while(!(USART1->ISR & USART_ISR_TXE));
    USART1->TDR = ch;
}
void usart_sendn(const uint8_t *str, int L){
    if(!str || L < 0) return;
    for(int i = 0; i < L; ++i){
        usart_putchar(str[i]);
    }
}

// setup all USARTs
void usarts_setup(){
    // clock
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_USART3EN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_EnableIRQ(USART3_IRQn);
}

void usart_config(uint8_t usartNo, usb_LineCoding *lc){
    if(!usartNo || usartNo > USARTSNO) return;
    speeds[usartNo] = lc->dwDTERate;
    volatile USART_TypeDef *U = USARTx[usartNo];
    U->ICR = 0xffffffff; // clear all flags
    U->BRR = SysFreq / lc->dwDTERate;
    U->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_RXNEIE; // 1start,8data,nstop; enable Rx,Tx,USART
    uint32_t tmout = 16000000;
    while(!(U->ISR & USART_ISR_TC)){if(--tmout == 0) break;} // polling idle frame Transmission
    U->ICR = 0xffffffff; // clear all flags again
}

void usart1_exti25_isr(){
    if(USART1->ISR & USART_ISR_RXNE){ // RX not emty - receive next char
        // read RDR clears flag
        uint8_t rb = USART1->RDR;
        USB_putbyte(1, rb);
    }
}
