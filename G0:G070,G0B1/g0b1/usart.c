/*
 * This file is part of the test project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "hardware.h" // Tms
#include "ringbuffer.h"
#include "usart.h"

// We do not use a ring buffer here because we expect incoming information
//   to flow slowly from the terminal.

// USART-depending part ------->
// select USART and its DMA channels
#define USARTx      USART1
#define USARTxAPB   APBENR2
#define USARTxEN    RCC_APBENR2_USART1EN
#define USART_APBEN RCC_APB1
// DMAMUX channels: 50 - USART1Rx, 51 - USART1Tx
#define DMAMUXRXN   (50)
#define DMAMUXTXN   (51)
// DMA channels: 2 (1 in MUX) - Rx, 3 (2 in MUX) - Tx; TC and error flags
// use DMA ch2/3 because they both have single IRQ
#define DMAx        DMA1
#define DMAxEN      RCC_AHBENR_DMA1EN
#define DMACHRX     DMA1_Channel2
#define DMARXTCF    DMA_ISR_TCIF2
#define DMARXEF     DMA_ISR_TEIF2
#define DMACHTX     DMA1_Channel3
#define DMATXTCF    DMA_ISR_TCIF3
#define DMATXEF     DMA_ISR_TEIF3
#define DMAMUXRX    DMAMUX1_Channel1
#define DMAMUXTX    DMAMUX1_Channel2
#define USARTIRQn   USART1_IRQn
#define DMAIRQ      DMA1_Channel2_3_IRQn
// interrupt aliases
static void usart_isr();
static void dma_isr();
void usart1_isr() __attribute__ ((alias ("usart_isr")));
void dma1_channel2_3_isr() __attribute__ ((alias ("dma_isr")));
// <-------- USART-depending part

// RX/TX DMA->CCR without EN flag
#define DMARXCCR    (DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_TEIE)
#define DMATXCCR    (DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE | DMA_CCR_TEIE)

static volatile uint8_t txrdy = 1, rxrdy = 0; // transmission done, next line received
static volatile USART_flags_t curflags; // current flags (cleared in `usart_process`)
static volatile uint8_t rbufno = 0; // current buf number
static uint8_t rbuf[USARTRXBUFSZ][2];
static uint8_t txbuf[USARTTXBUFSZ]; // for ringbuffer
static ringbuffer TxRB = {.data = txbuf, .length = USARTTXBUFSZ};

char *usart_getline(){
    if(!rxrdy) return NULL;
    rxrdy = 0; // clear ready flag
    return (char*)rbuf[!rbufno]; // current buffer is in filling stage, return old - filled - buffer
}

#define USART_BRR(speed)    ((SysFreq + speed/2) / speed)

void usart_setup(uint32_t speed){
    RCC->AHBENR |= DMAxEN; // enable DMA
    // enable USART clocking
    RCC->USARTxAPB |= USARTxEN;
    // baudrate
    USARTx->BRR = USART_BRR(speed);
    // eol character: '/n'
    USARTx->CR2 = USART_CR2_ADD_VAL('\n');
    // enable DMA transmission
    USARTx->CR3 = USART_CR3_DMAT | USART_CR3_DMAR;
    // set up DMA channels
    // Tx channel: mem++, mem->periph, 8bit, compl.&err. irq
    DMACHTX->CCR = DMATXCCR;
    DMACHTX->CPAR = (uint32_t) &USARTx->TDR; // peripherial address
    // Rx channel: mem++, periph->mem, 8bit, compl.&err. irq
    DMACHRX->CCR = DMARXCCR;
    DMACHRX->CPAR = (uint32_t) &USARTx->RDR; // peripherial address
    DMACHRX->CNDTR = USARTRXBUFSZ;
    DMACHRX->CMAR = (uint32_t)&rbuf[rbufno];
    // set up DMAMUX channels
    // enumeration of DMAMUX starts from 0 (DMA - from 1)!
    DMAMUXRX->CCR = DMAMUXRXN;
    DMAMUXTX->CCR = DMAMUXTXN;
    // charmatch interrupt, enable transmitter and receiver, enable usart
    USARTx->CR1 = USART_CR1_CMIE | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
    USARTx->ICR = 0xffffffff; // clear all flags
    DMACHRX->CCR = DMARXCCR | DMA_CCR_EN; // start receiving right now
    NVIC_EnableIRQ(USARTIRQn);
    NVIC_EnableIRQ(DMAIRQ);
}

/**
 * @brief usart_sendbuf - send next data portion
 * @return TRUE if sent something
 */
static int usart_sendbuf(){
    static uint8_t dmatxbuf[USARTTXDMABUFSZ];
    if(!txrdy) return FALSE;
    int rd = RB_read(&TxRB, dmatxbuf, USARTTXDMABUFSZ);
    if(rd < 1) return FALSE; // nothing to write or busy
    // set up DMA
    DMACHTX->CCR = DMATXCCR;
    DMACHTX->CMAR = (uint32_t) dmatxbuf;
    DMACHTX->CNDTR = rd;
    USARTx->ICR = USART_ICR_TCCF; // clear TC flag
    txrdy = 0;
    // activate DMA
    DMACHTX->CCR = DMATXCCR | DMA_CCR_EN;
    return TRUE;
}

int usart_send(const char *str, int len){
    if(!str || len < 1) return 0;
    uint32_t t = Tms;
    int sent = 0;
    do{
        IWDG->KR = IWDG_REFRESH;
        int put = RB_write(&TxRB, (uint8_t*)str, len);
        if(put < 0) continue; // busy
        else if(put == 0) usart_sendbuf(); // no place
        else{
            len -= put;
            sent += put;
            str += put;
        }
    }while(len && (Tms - t) < USARTBLKTMOUT); // not more than `block` ms!
    return sent;
}

int usart_sendstr(const char *str){
    int l = strlen(str);
    return usart_send(str, l);
}

// return current flags
USART_flags_t usart_process(){
    static uint32_t Tlast = 0;
    USART_flags_t flags = curflags;
    curflags.all = 0;
    if(RB_datalento(&TxRB, '\n') > 1 || Tms - Tlast >= USARTSENDTMOUT){ // send buffer as we found '\n' or each 10ms
        if(usart_sendbuf()) Tlast = Tms;
    }
    return flags;
}

// interrupt by '\n'
static void usart_isr(){
    if(USARTx->ISR & USART_ISR_CMF){ // got '\n' @ USARTx
        DMACHRX->CCR = DMARXCCR;
        rxrdy = 1;
        int l = USARTRXBUFSZ - DMACHRX->CNDTR - 1; // strlen without '\n'
        rbuf[rbufno][l] = 0; // throw out '\n'
        rbufno = !rbufno; // prepare next buffer to receive
        // reload DMA Rx with next buffer
        DMACHRX->CMAR = (uint32_t) rbuf[rbufno];
        DMACHRX->CNDTR = USARTRXBUFSZ;
        DMACHRX->CCR = DMARXCCR | DMA_CCR_EN;
    }
    USARTx->ICR = 0xffffffff; // clear all flags
}

// ch2 - Tx, ch3 - Rx
static void dma_isr(){
    volatile uint32_t isr = DMAx->ISR;
    if(isr & DMATXTCF) txrdy = 1;
    if(isr & DMATXEF) curflags.txerr = 1;
    if(isr & (DMARXTCF | DMARXEF)){ // receive complete or error -> buffer overflow
        if(rbuf[rbufno][USARTRXBUFSZ-1] != '\n'){ // last symbol is not a newline
            curflags.rxovrfl = 1;
            DMACHRX->CCR = DMARXCCR; // stop to reload
            DMACHRX->CNDTR = USARTRXBUFSZ;
            DMACHRX->CMAR = (uint32_t) rbuf[rbufno];
            DMACHRX->CCR = DMARXCCR | DMA_CCR_EN;
        }
    }
    DMAx->IFCR = 0xffffffff; // clear all flags
}
