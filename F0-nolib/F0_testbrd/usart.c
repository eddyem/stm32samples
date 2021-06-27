/*
 * usart.c
 *
 * Copyright 2017 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 */
#include "stm32f0.h"
#include "hardware.h"
#include "proto.h"
#include "usart.h"
#include "usb.h"
#include <string.h>

extern volatile uint32_t Tms;
// USART tx DMA 1: DMA1_Channel2, 2: DMA1_Channel4, 3: DMA1_Channel7
static DMA_Channel_TypeDef *USARTDMA[USARTNUM] = {
    DMA1_Channel2, DMA1_Channel4
#ifdef USART3
    ,DMA1_Channel7
#endif
};
static USART_TypeDef *USARTs[USARTNUM] = {
    USART1, USART2
#ifdef USART3
    ,USART3
#endif
};

static volatile int idatalen[USARTNUM][2] = {0}; // received data line length (including '\n')
static volatile int odatalen[USARTNUM][2] = {0};

volatile int linerdy[USARTNUM] = {0}, // received data ready
    dlen[USARTNUM] = {0},             // length of data (including '\n') in current buffer
    bufovr[USARTNUM] = {0},           // input buffer overfull
    txrdy[USARTNUM] = {1,1             // transmission done
#ifdef USART3
        ,1
#endif
    }
;


int rbufno[USARTNUM] = {0}, tbufno[USARTNUM] = {0}; // current rbuf/tbuf numbers
static char rbuf[USARTNUM][2][UARTBUFSZI], tbuf[USARTNUM][2][UARTBUFSZO]; // receive & transmit buffers
static char *recvdata[USARTNUM] = {0};

/**
 * return length of received data (without trailing zero
 * usartno: 1, 2 or 3
 */
int usart_getline(int usartno, char **line){
    --usartno;
    if(bufovr[usartno]){
        bufovr[usartno] = 0;
        linerdy[usartno] = 0;
        return 0;
    }
    *line = recvdata[usartno];
    linerdy[usartno] = 0;
    return dlen[usartno];
}

// transmit current tbuf for all USARTs and swap buffers
void transmit_tbuf(){
    for(int usartno = 0; usartno < USARTNUM; ++usartno){
        uint32_t p = 1000000;
        while(!txrdy[usartno] && --p);
        if(!txrdy[usartno]) continue;
        register int l = odatalen[usartno][tbufno[usartno]];
        if(!l) continue;
        txrdy[usartno] = 0;
        odatalen[usartno][tbufno[usartno]] = 0;
        USARTDMA[usartno]->CCR &= ~DMA_CCR_EN;
        USARTDMA[usartno]->CMAR = (uint32_t) tbuf[usartno][tbufno[usartno]]; // mem
        USARTDMA[usartno]->CNDTR = l;
        USARTDMA[usartno]->CCR |= DMA_CCR_EN;
        tbufno[usartno] = !tbufno[usartno];
    }
}

void usart_putchar(int usartno, const char ch){
    --usartno;
    if(odatalen[usartno][tbufno[usartno]] == UARTBUFSZO) transmit_tbuf();
    tbuf[usartno][tbufno[usartno]][odatalen[usartno][tbufno[usartno]]++] = ch;
}

void usart_send(int usartno, const char *str){
    --usartno;
    while(*str){
        if(odatalen[usartno][tbufno[usartno]] == UARTBUFSZO) transmit_tbuf();
        tbuf[usartno][tbufno[usartno]][odatalen[usartno][tbufno[usartno]]++] = *str++;
    }
}

void usart_sendn(int usartno, const char *str, uint32_t L){
    --usartno;
    for(uint32_t i = 0; i < L; ++i){
        if(odatalen[usartno][tbufno[usartno]] == UARTBUFSZO) transmit_tbuf();
        tbuf[usartno][tbufno[usartno]][odatalen[usartno][tbufno[usartno]]++] = *str++;
    }
}

void newline(int usartno){
    usart_putchar(usartno, '\n');
    transmit_tbuf();
}


void usart_setup(){
    // USART1: Rx - PA10, Tx - PA9  (AF1)
    // USART2: Rx - PA3, Tx - PA2   (AF1)
    // USART3: Rx - PB11, Tx - PB10 (AF4)
    // setup pins:
    GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER2|GPIO_MODER_MODER3|GPIO_MODER_MODER9 | GPIO_MODER_MODER10)) |
                   GPIO_MODER_MODER2_AF | GPIO_MODER_MODER3_AF | GPIO_MODER_MODER9_AF | GPIO_MODER_MODER10_AF;
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(GPIO_AFRL_AFRL2 | GPIO_AFRL_AFRL3)) |
                1 << (2 * 4) | 1 << (3 * 4); // PA2,3
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(GPIO_AFRH_AFRH1 | GPIO_AFRH_AFRH2)) |
                1 << (1 * 4) | 1 << (2 * 4); // PA9, PA10
    // clock
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
#ifdef USART3
    GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODER10 | GPIO_MODER_MODER11)) |
                   GPIO_MODER_MODER10_AF | GPIO_MODER_MODER11_AF;
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~(GPIO_AFRH_AFRH2 | GPIO_AFRH_AFRH3)) |
            4 << (2 * 4) | 4 << (3 * 4); // PB10, PB11
#endif
    for(int i = 0; i < USARTNUM; ++i){
        USARTs[i]->ICR = 0xffffffff; // clear all flags
        // USARTX Tx DMA
        USARTDMA[i]->CPAR = (uint32_t) &USARTs[i]->TDR; // periph
        USARTDMA[i]->CCR |= DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE; // 8bit, mem++, mem->per, transcompl irq
        // setup usarts
        USARTs[i]->BRR = 480000 / 1152;
        USARTs[i]->CR3 = USART_CR3_DMAT; // enable DMA Tx
        USARTs[i]->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_RXNEIE; // 1start,8data,nstop; enable Rx,Tx,USART
        uint32_t tmout = 16000000;
        while(!(USARTs[i]->ISR & USART_ISR_TC)){if(--tmout == 0) break;} // polling idle frame Transmission
        USARTs[i]->ICR = 0xffffffff; // clear all flags again
    }
    NVIC_SetPriority(DMA1_Channel2_3_IRQn, 3);
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    NVIC_SetPriority(USART1_IRQn, 0);
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, 3);
    NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);
    NVIC_SetPriority(USART2_IRQn, 0);
    NVIC_EnableIRQ(USART2_IRQn);
#ifdef USART3
    NVIC_SetPriority(USART3_4_IRQn, 0);
    NVIC_EnableIRQ(USART3_4_IRQn);
#endif
}


static void usart_IRQ(int usartno){
    USART_TypeDef *USARTX = USARTs[usartno];
    //USND("USART"); USB_sendstr(u2str(usartno+1)); USND(" IRQ, ISR=");
    //USB_sendstr(uhex2str(USARTX->ISR)); USND("\n");
    #ifdef CHECK_TMOUT
    static uint32_t tmout[USARTNUM] = {0};
    #endif
    if(USARTX->ISR & USART_ISR_RXNE){ // RX not emty - receive next char
        #ifdef CHECK_TMOUT
        if(tmout[usartno] && Tms >= tmout[usartno]){ // set overflow flag
            bufovr[usartno] = 1;
            idatalen[usartno][rbufno[usartno]] = 0;
        }
        tmout[usartno] = Tms + TIMEOUT_MS;
        if(!tmout[usartno]) tmout[usartno] = 1; // prevent 0
        #endif
        // read RDR clears flag
        uint8_t rb = USARTX->RDR;
        //USND("RB="); USB_sendstr(uhex2str(rb)); USND("\n");
        if(idatalen[usartno][rbufno[usartno]] < UARTBUFSZI){ // put next char into buf
            rbuf[usartno][rbufno[usartno]][idatalen[usartno][rbufno[usartno]]++] = rb;
            if(rb == '\n'){ // got newline - line ready
                //USND("Newline\n");
                linerdy[usartno] = 1;
                dlen[usartno] = idatalen[usartno][rbufno[usartno]];
                recvdata[usartno] = rbuf[usartno][rbufno[usartno]];
                // prepare other buffer
                rbufno[usartno] = !rbufno[usartno];
                idatalen[usartno][rbufno[usartno]] = 0;
                #ifdef CHECK_TMOUT
                // clear timeout at line end
                tmout[usartno] = 0;
                #endif
            }
        }else{ // buffer overrun
            bufovr[usartno] = 1;
            idatalen[usartno][rbufno[usartno]] = 0;
            #ifdef CHECK_TMOUT
            tmout[usartno] = 0;
            #endif
        }
    }
    USARTX->ICR = 0xffffffff;
}

void usart1_isr(){
    usart_IRQ(0);
}

void usart2_isr(){
    usart_IRQ(1);
}


// work with USART3 only @ boards that have it
#ifdef USART3
void usart3_4_isr(){
    usart_IRQ(2);
}
#endif

// USART1
void dma1_channel2_3_isr(){
    if(DMA1->ISR & DMA_ISR_TCIF2){ // Tx
        DMA1->IFCR |= DMA_IFCR_CTCIF2; // clear TC flag
        txrdy[0] = 1;
    }
}
// USART2 + USART3
void dma1_channel4_5_isr(){
    if(DMA1->ISR & DMA_ISR_TCIF4){ // Tx
        DMA1->IFCR |= DMA_IFCR_CTCIF4; // clear TC flag
        txrdy[1] = 1;
    }
#ifdef USART3
    if(DMA1->ISR & DMA_ISR_TCIF7){ // Tx
        DMA1->IFCR |= DMA_IFCR_CTCIF7; // clear TC flag
        txrdy[2] = 1;
    }
#endif
}
