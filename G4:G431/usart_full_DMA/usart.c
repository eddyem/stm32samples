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

#include <stm32g4.h>
#include <string.h>

#include "hardware.h" // Tms
#include "ringbuffer.h"
#include "usart.h"

// USART for text-based protocol, each data portion ends with '\n'
// RX works over circular DMA

// USART-depending part ------->
// USART1 @ PA9 (Tx) - MUX25 - and PA10 (Rx) - MUX24
// select USART and its DMA channels
#define USARTx      USART1
#define USARTxAPB   APB2ENR
#define USARTxEN    RCC_APB2ENR_USART1EN
#define USART_APBEN RCC_APB2
// DMAMUX channels: 24 - USART1Rx, 25 - USART1Tx
#define DMAMUXRXN   (24)
#define DMAMUXTXN   (25)
// DMA channels: 1 (0 in MUX) - Rx, 2 (1 in MUX) - Tx; TC and error flags
// use DMA ch2/3 because they both have single IRQ
#define DMAx        DMA1
#define DMAxEN      (RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN)
#define DMACHRX     DMA1_Channel1
#define DMARXTCF    DMA_ISR_TCIF1
#define DMARXEF     DMA_ISR_TEIF1
#define DMACHTX     DMA1_Channel2
#define DMATXTCF    DMA_ISR_TCIF2
#define DMATXEF     DMA_ISR_TEIF2
#define DMAMUXRX    DMAMUX1_Channel0
#define DMAMUXTX    DMAMUX1_Channel1
#define USARTIRQn   USART1_IRQn
#define DMARXIRQ    DMA1_Channel1_IRQn
#define DMATXIRQ    DMA1_Channel2_IRQn

// interrupt aliases
static void usart_isr();
static void dmatx_isr();
static void dmarx_isr();
void usart1_isr() __attribute__ ((alias ("usart_isr")));
void dma1_channel1_isr() __attribute__ ((alias ("dmarx_isr")));
void dma1_channel2_isr() __attribute__ ((alias ("dmatx_isr")));
// <-------- USART-depending part

// RX/TX DMA->CCR without EN flag
#define DMARXCCR    (DMA_CCR_MINC | DMA_CCR_CIRC | DMA_CCR_TEIE)
#define DMATXCCR    (DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE | DMA_CCR_TEIE)

static volatile bool gotstring = true; // got '\n' in input stream -> force data reading to inbuf
static volatile bool txrdy = true; // Tx DMA not busy
static volatile USART_flags_t curflags; // current flags (cleared in `usart_process`)
// rx/tx DMA buffers
static uint8_t dmarxbuf[USARTRXDMABUFSZ];
static uint8_t dmatxbuf[USARTTXDMABUFSZ];
// index of last DMA read position
static uint32_t dma_read_idx = 0;
// for ringbuffer
static uint8_t rbrxbuf[USARTRXBUFSZ], rbtxbuf[USARTTXBUFSZ];
static ringbuffer TxRB = {.data = rbtxbuf, .length = USARTTXBUFSZ};
static ringbuffer RxRB = {.data = rbrxbuf, .length = USARTRXBUFSZ};

#define USART_BRR(speed)    ((SysFreq + (speed)/2) / (speed))

static void reinit_rx_dma(){
    dma_read_idx = 0;
    RB_clearbuf(&RxRB);
    DMACHRX->CCR = DMARXCCR; // stop to reload
    DMACHRX->CNDTR = USARTRXDMABUFSZ;
    DMACHRX->CMAR = (uint32_t) dmarxbuf;
    DMACHRX->CCR = DMARXCCR | DMA_CCR_EN;
}

void usart_setup(uint32_t speed){
    RCC->AHB1ENR |= DMAxEN; // enable DMA
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
    // set up DMAMUX channels
    // enumeration of DMAMUX starts from 0 (DMA - from 1)!
    DMAMUXRX->CCR = DMAMUXRXN;
    DMAMUXTX->CCR = DMAMUXTXN;
    // charmatch interrupt, enable transmitter and receiver, enable usart
    USARTx->CR1 = USART_CR1_CMIE | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
    USARTx->ICR = 0xffffffff; // clear all flags
    reinit_rx_dma();
    NVIC_EnableIRQ(USARTIRQn);
    NVIC_EnableIRQ(DMARXIRQ);
    NVIC_EnableIRQ(DMATXIRQ);
}

/**
 * @brief usart_sendbuf - send next data portion
 * @return TRUE if sent something
 */
static bool usart_sendbuf(){
    if(!txrdy) return false;
    int rd = RB_read(&TxRB, dmatxbuf, USARTTXDMABUFSZ);
    if(rd < 1) return false; // nothing to write or busy
    // set up DMA
    DMACHTX->CCR = DMATXCCR;
    DMACHTX->CMAR = (uint32_t) dmatxbuf;
    DMACHTX->CNDTR = rd;
    USARTx->ICR = USART_ICR_TCCF; // clear TC flag
    txrdy = false;
    // activate DMA
    DMACHTX->CCR = DMATXCCR | DMA_CCR_EN;
    return true;
}

int usart_send(const char *str, int len){
    if(!str || len < 1) return 0;
    uint32_t t = Tms;
    int sent = 0;
    do{
        IWDG->KR = IWDG_REFRESH;
        int put = RB_write(&TxRB, (uint8_t*)str, len);
        if(put < 0) continue; // busy
        else if(put == 0){
            usart_sendbuf(); // no place
        }else{
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
    int remained = DMACHRX->CNDTR;
    int write_idx = USARTRXDMABUFSZ - remained; // next symbol to be written
    int available = (write_idx - dma_read_idx); // length of data available
    if(available < 0) available += USARTRXDMABUFSZ; // write to the left of read
    if(available == 0) return flags;
    // add next data portion to RX ring buffer
    if(available >= (USARTRXDMABUFSZ / 2) || gotstring){
        // copy data in one or two chunks (wrap handling)
        bool wrOK = false;
        // check if we can write to RB `available` bytes
        int rballow = RxRB.length - 1 - RB_datalen(&RxRB);
        if(rballow < available){
            if(available > USARTRXDMABUFSZ - 2){ // near overfull
                flags.rxovrfl = 1;
                reinit_rx_dma();
                RB_clearbuf(&RxRB);
                return flags;
            }
            if(rballow < 1) return flags;
            available = rballow; // read at least as we can
        }
        if(dma_read_idx + available <= USARTRXDMABUFSZ){ // head before tail
            if(available == RB_write(&RxRB, &dmarxbuf[dma_read_idx], available)) wrOK = true;
        }else{ // head after tail - two chunks
            int first = USARTRXDMABUFSZ - dma_read_idx;
            if((first == RB_write(&RxRB, &dmarxbuf[dma_read_idx], first)) &&
                (available - first) == RB_write(&RxRB, dmarxbuf, available - first)) wrOK = true;
        }
        if(wrOK){
            gotstring = 0;
            dma_read_idx = write_idx; // update read pointer
        }
    }
    if(RB_hasbyte(&RxRB, '\n')) flags.gotstring = 1;
    return flags;
}

char *usart_getline(){
    static char buff[256];
    int l = RB_datalento(&RxRB, '\n');
    if(l < 1){
        l = RB_datalen(&RxRB); // Rx ringbuffer could be near overflow but without '\n'
        if(l < 255) return NULL; // allow to wait for last symbols
    }
    if(l > 255){ // overflow -> read at least part of the string
        l = 255;
    }
    if(l != RB_read(&RxRB, (uint8_t*)buff, l)) return NULL;
    buff[l] = 0; // return with '\n' at end of line (so user can detect non-finished overflowed lines)
    return buff;
}

// interrupt by '\n'
static void usart_isr(){
    if(USARTx->ISR & USART_ISR_CMF){ // got '\n' @ USARTx
        gotstring = true;
    }
    USARTx->ICR = 0xffffffff; // clear all flags
}

static void dmarx_isr(){
    volatile uint32_t isr = DMAx->ISR;
    if(isr & DMARXEF){ // error
        reinit_rx_dma();
        curflags.rxovrfl = 1;
    }
    DMAx->IFCR = DMARXEF; // clear all flags
}

static void dmatx_isr(){
    volatile uint32_t isr = DMAx->ISR;
    if(isr & DMATXTCF) txrdy = true;
    if(isr & DMATXEF) curflags.txerr = 1;
    DMAx->IFCR = DMATXTCF | DMATXEF; // clear all flags
}
