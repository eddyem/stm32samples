/*
 * This file is part of the rtc project.
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

#include <stm32g0.h>
#include <string.h>

#include "usart.h"

// RX/TX DMA->CCR without EN flag
#define DMARXCCR    (DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_TEIE)
#define DMATXCCR    (DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE | DMA_CCR_TEIE)

volatile int u3txrdy = 1, u3rxrdy = 0; // transmission done, next line received
static volatile int bufovr = 0, wasbufovr = 0; // Rx buffer overflow or error flag -> delete next line
static volatile int rbufno = 0, tbufno = 0; // current buf number
static volatile char rbuf[2][UARTBUFSZ], tbuf[2][UARTBUFSZ]; // receive & transmit buffers
static volatile int rxlen[2] = {0}, txlen[2] = {0};

char *usart3_getline(int *wasbo){
    if(wasbo) *wasbo = wasbufovr;
    wasbufovr = 0;
    if(!u3rxrdy) return NULL;
    u3rxrdy = 0; // clear ready flag
    return (char*)rbuf[!rbufno]; // current buffer is in filling stage, return old - filled - buffer
}

#define USART_BRR(speed)    ((64000000 + speed/2) / speed)

// USART3 @ PD8 (Tx) and PD9 (Rx) - both AF0
void usart3_setup(){
    RCC->IOPENR |= RCC_IOPENR_GPIODEN; // enable PD
    RCC->AHBENR |= RCC_AHBENR_DMA1EN; // enable DMA1
    // set PD8 and PD9 as AF
    GPIOD->MODER = (GPIOD->MODER & ~(GPIO_MODER_MODE8 | GPIO_MODER_MODE9))
            | GPIO_MODER_MODER8_AF | GPIO_MODER_MODER9_AF;
    // AF0 for USART3 @ PD8/PD9
    GPIOD->AFR[1] = GPIOD->AFR[1] & ~(GPIO_AFRH_AFSEL8 | GPIO_AFRH_AFSEL9);
    // enable USART3 clocking
    RCC->APBENR1 |= RCC_APBENR1_USART3EN;
    // baudrate 115200
    USART3->BRR = USART_BRR(115200);
    // eol character: '/n'
    USART3->CR2 = USART_CR2_ADD_VAL('\n');
    // enable DMA transmission
    USART3->CR3 = USART_CR3_DMAT | USART_CR3_DMAR;
    // set up DMA channels: 2 - Tx, 3 - Rx
    // Tx channel: mem++, mem->periph, 8bit, compl.&err. irq
    DMA1_Channel2->CCR = DMATXCCR;
    DMA1_Channel2->CPAR = (uint32_t) &USART3->TDR; // peripherial address
    // Rx channel: mem++, periph->mem, 8bit, compl.&err. irq
    DMA1_Channel3->CCR = DMARXCCR;
    DMA1_Channel3->CPAR = (uint32_t) &USART3->RDR; // peripherial address
    DMA1_Channel3->CNDTR = UARTBUFSZ;
    DMA1_Channel3->CMAR = (uint32_t)&rbuf[rbufno];
    // set up DMAMUX channels: 55 - USART3_TX, 54 - USART3_RX
    // enumeration of DMAMUX starts from 0 (DMA - from 1)!
    DMAMUX1_Channel1->CCR = 55;
    DMAMUX1_Channel2->CCR = 54;
    // charmatch interrupt, enable transmitter and receiver, enable usart
    USART3->CR1 = USART_CR1_CMIE | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
    USART3->ICR = 0xffffffff; // clear all flags
    DMA1_Channel3->CCR = DMARXCCR | DMA_CCR_EN; // start receiving right now
    NVIC_EnableIRQ(USART3_4_IRQn);
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
}

static int usart3_sendN(const char *str, int len){
    int rest = UARTBUFSZ - txlen[tbufno];
    if(rest == 0 && !u3txrdy) return 0; // buffer is full while transmission in process
    if(len < rest) rest = len;
    memcpy((char*)(tbuf[tbufno] + txlen[tbufno]), str, rest);
    txlen[tbufno] += rest;
    if(!u3txrdy) return rest;
    if(txlen[tbufno] == UARTBUFSZ) usart3_sendbuf();
    if(rest == len) return len;
    len -= rest;
    // now fill another - empty - buffer
    if(len > UARTBUFSZ) len = UARTBUFSZ;
    memcpy((char*)tbuf[tbufno], str + rest, len);
    txlen[tbufno] = len;
    return rest + len;
}

void usart3_send(const char *str, int len){
    while(len){
        int sent = usart3_sendN(str, len);
        str += sent;
        len -= sent;
    }
}

void usart3_sendstr(const char *str){
    int l = strlen(str);
    usart3_send(str, l);
}

/**
 * @brief usart3_sendbuf - send current buffer
 */
void usart3_sendbuf(){
    if(!u3txrdy || txlen[tbufno] == 0) return;
    // set up DMA
    DMA1_Channel2->CCR = DMATXCCR;
    DMA1_Channel2->CMAR = (uint32_t)&tbuf[tbufno];
    DMA1_Channel2->CNDTR = txlen[tbufno];
    USART3->ICR = USART_ICR_TCCF; // clear TC flag
    u3txrdy = 0;
    // activate DMA
    DMA1_Channel2->CCR = DMATXCCR | DMA_CCR_EN;
    tbufno = !tbufno; // swap buffers
    txlen[tbufno] = 0;
}

// return amount of bytes sents
int usart3_send_blocking(const char *str, int len){
    if(!u3txrdy) return 0;
    USART3->CR1 |= USART_CR1_TE;
    for(int i = 0; i < len; ++i){
        while(!(USART3->ISR & USART_ISR_TXE_TXFNF));
        USART3->TDR = *str++;
    }
    return len;
}

// interrupt by '\n'
void usart3_4_isr(){
    if(USART3->ISR & USART_ISR_CMF){ // got '\n' @ USART3
        DMA1_Channel3->CCR = DMARXCCR;
        if(!bufovr){ // forget about broken line @ buffer overflow
            u3rxrdy = 1;
            int l = UARTBUFSZ - DMA1_Channel3->CNDTR - 1; // strlen
            rxlen[rbufno] = l;
            rbuf[rbufno][l] = 0;
            rbufno = !rbufno;
        }else{
            bufovr = 0;
            wasbufovr = 1;
        }
        // reload DMA Rx with next buffer
        DMA1_Channel3->CMAR = (uint32_t)&rbuf[rbufno];
        DMA1_Channel3->CNDTR = UARTBUFSZ;
        DMA1_Channel3->CCR = DMARXCCR | DMA_CCR_EN;
    }
    USART3->ICR = 0xffffffff; // clear all flags
}

// ch2 - Tx, ch3 - Rx
void dma1_channel2_3_isr(){
    uint32_t isr = DMA1->ISR;
    if(isr & (DMA_ISR_TCIF2 | DMA_ISR_TEIF2)){ // transfer complete or error
        u3txrdy = 1;
    }
    if(isr & (DMA_ISR_TCIF3 | DMA_ISR_TEIF3)){ // receive complete or error -> buffer overflow
        if(rbuf[rbufno][UARTBUFSZ-1] != '\n'){ // last symbol is not a newline
            bufovr = 1;
            DMA1_Channel3->CCR = DMARXCCR;
            DMA1_Channel3->CNDTR = UARTBUFSZ;
            DMA1_Channel3->CCR = DMARXCCR | DMA_CCR_EN;
        }
    }
    DMA1->IFCR = 0xff0; // clear all flags for 2&3
}
