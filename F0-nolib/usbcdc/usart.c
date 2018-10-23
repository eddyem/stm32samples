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
#include "usart.h"
#include <string.h>

extern volatile uint32_t Tms;
static volatile int idatalen[2] = {0,0}; // received data line length (including '\n')
static volatile int odatalen[2] = {0,0};

volatile int linerdy = 0,        // received data ready
    dlen = 0,           // length of data (including '\n') in current buffer
    bufovr = 0,         // input buffer overfull
    txrdy = 1           // transmission done
;


int rbufno = 0, tbufno = 0; // current rbuf/tbuf numbers
static char rbuf[2][UARTBUFSZI], tbuf[2][UARTBUFSZO]; // receive & transmit buffers
static char *recvdata = NULL;

/**
 * return length of received data (without trailing zero
 */
int usart_getline(char **line){
    if(bufovr){
        bufovr = 0;
        linerdy = 0;
        return 0;
    }
    *line = recvdata;
    linerdy = 0;
    return dlen;
}

// transmit current tbuf and swap buffers
void transmit_tbuf(){
    uint32_t tmout = 16000000;
    while(!txrdy){if(--tmout == 0) break;}; // wait for previos buffer transmission
    register int l = odatalen[tbufno];
    if(!l) return;
    txrdy = 0;
    odatalen[tbufno] = 0;
#if USARTNUM == 2
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;
    DMA1_Channel4->CMAR = (uint32_t) tbuf[tbufno]; // mem
    DMA1_Channel4->CNDTR = l;
    DMA1_Channel4->CCR |= DMA_CCR_EN; // start transmission
#elif USARTNUM == 1
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    DMA1_Channel2->CMAR = (uint32_t) tbuf[tbufno]; // mem
    DMA1_Channel2->CNDTR = l;
    DMA1_Channel2->CCR |= DMA_CCR_EN;
#else
#error "Not implemented"
#endif
    tbufno = !tbufno;
}

void usart_putchar(const char ch){
    if(odatalen[tbufno] == UARTBUFSZO) transmit_tbuf();
    tbuf[tbufno][odatalen[tbufno]++] = ch;
}

void usart_send(const char *str){
    uint32_t x = 512;
    while(*str && --x){
        if(odatalen[tbufno] == UARTBUFSZO) transmit_tbuf();
        tbuf[tbufno][odatalen[tbufno]++] = *str++;
    }
}

void newline(){
    usart_putchar('\n');
    transmit_tbuf();
}


void usart_setup(){
// Nucleo's USART2 connected to VCP proxy of st-link
    uint32_t tmout = 16000000;
#if USARTNUM == 2
    // setup pins: PA2 (Tx - AF1), PA15 (Rx - AF1)
    // AF mode (AF1)
    GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER2|GPIO_MODER_MODER15))\
                | (GPIO_MODER_MODER2_AF | GPIO_MODER_MODER15_AF);
    GPIOA->AFR[0] = (GPIOA->AFR[0] &~GPIO_AFRH_AFRH2) | 1 << (2 * 4); // PA2
    GPIOA->AFR[1] = (GPIOA->AFR[1] &~GPIO_AFRH_AFRH7) | 1 << (7 * 4); // PA15
    // DMA: Tx - Ch4
    DMA1_Channel4->CPAR = (uint32_t) &USART2->TDR; // periph
    DMA1_Channel4->CCR |= DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE; // 8bit, mem++, mem->per, transcompl irq
    // Tx CNDTR set @ each transmission due to data size
    NVIC_SetPriority(DMA1_Channel4_5_IRQn, 3);
    NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);
    NVIC_SetPriority(USART2_IRQn, 0);
    // setup usart2
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN; // clock
    // oversampling by16, 115200bps (fck=48mHz)
    //USART2_BRR = 0x1a1; // 48000000 / 115200
    USART2->BRR = 480000 / 1152;
    USART2->CR3 = USART_CR3_DMAT; // enable DMA Tx
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; // 1start,8data,nstop; enable Rx,Tx,USART
    while(!(USART2->ISR & USART_ISR_TC)){if(--tmout == 0) break;} // polling idle frame Transmission
    USART2->ICR |= USART_ICR_TCCF; // clear TC flag
    USART2->CR1 |= USART_CR1_RXNEIE;
    NVIC_EnableIRQ(USART2_IRQn);
// USART1 of main board
#elif USARTNUM == 1
    // PA9 - Tx, PA10 - Rx (AF1)
    GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER9 | GPIO_MODER_MODER10))\
                | (GPIO_MODER_MODER9_AF | GPIO_MODER_MODER10_AF);
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(GPIO_AFRH_AFRH1 | GPIO_AFRH_AFRH2)) |
                1 << (1 * 4) | 1 << (2 * 4); // PA9, PA10
    // USART1 Tx DMA - Channel2 (default value in SYSCFG_CFGR1)
    DMA1_Channel2->CPAR = (uint32_t) &USART1->TDR; // periph
    DMA1_Channel2->CMAR = (uint32_t) tbuf; // mem
    DMA1_Channel2->CCR |= DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE; // 8bit, mem++, mem->per, transcompl irq
    // Tx CNDTR set @ each transmission due to data size
    NVIC_SetPriority(DMA1_Channel2_3_IRQn, 3);
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    NVIC_SetPriority(USART1_IRQn, 0);
    // setup usart1
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    USART1->BRR = 480000 / 1152;
    USART1->CR3 = USART_CR3_DMAT; // enable DMA Tx
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; // 1start,8data,nstop; enable Rx,Tx,USART
    while(!(USART1->ISR & USART_ISR_TC)){if(--tmout == 0) break;} // polling idle frame Transmission
    USART1->ICR |= USART_ICR_TCCF; // clear TC flag
    USART1->CR1 |= USART_CR1_RXNEIE;
    NVIC_EnableIRQ(USART1_IRQn);
#else
#error "Not implemented"
#endif
}

#if USARTNUM == 2
void usart2_isr(){
// USART1
#elif USARTNUM == 1
void usart1_isr(){
#else
#error "Not implemented"
#endif
    #ifdef CHECK_TMOUT
    static uint32_t tmout = 0;
    #endif
    if(USARTX->ISR & USART_ISR_RXNE){ // RX not emty - receive next char
        #ifdef CHECK_TMOUT
        if(tmout && Tms >= tmout){ // set overflow flag
            bufovr = 1;
            idatalen[rbufno] = 0;
        }
        tmout = Tms + TIMEOUT_MS;
        if(!tmout) tmout = 1; // prevent 0
        #endif
        // read RDR clears flag
        uint8_t rb = USARTX->RDR;
        if(idatalen[rbufno] < UARTBUFSZI){ // put next char into buf
            rbuf[rbufno][idatalen[rbufno]++] = rb;
            if(rb == '\n'){ // got newline - line ready
                linerdy = 1;
                dlen = idatalen[rbufno];
                recvdata = rbuf[rbufno];
                // prepare other buffer
                rbufno = !rbufno;
                idatalen[rbufno] = 0;
                #ifdef CHECK_TMOUT
                // clear timeout at line end
                tmout = 0;
                #endif
            }
        }else{ // buffer overrun
            bufovr = 1;
            idatalen[rbufno] = 0;
            #ifdef CHECK_TMOUT
            tmout = 0;
            #endif
        }
    }
}

// print 32bit unsigned int
void printu(uint32_t val){
    char bufa[11], bufb[10];
    int l = 0, bpos = 0;
    if(!val){
        bufa[0] = '0';
        l = 1;
    }else{
        while(val){
            bufb[l++] = val % 10 + '0';
            val /= 10;
        }
        int i;
        bpos += l;
        for(i = 0; i < l; ++i){
            bufa[--bpos] = bufb[i];
        }
    }
    bufa[l + bpos] = 0;
    usart_send(bufa);
}

// print 32bit unsigned int as hex
void printuhex(uint32_t val){
    usart_send("0x");
    uint8_t *ptr = (uint8_t*)&val + 3;
    int i, j;
    for(i = 0; i < 4; ++i, --ptr){
        for(j = 1; j > -1; --j){
            register uint8_t half = (*ptr >> (4*j)) & 0x0f;
            if(half < 10) usart_putchar(half + '0');
            else usart_putchar(half - 10 + 'a');
        }
    }
}

// dump memory buffer
void hexdump(uint8_t *arr, uint16_t len){
    for(uint16_t l = 0; l < len; ++l, ++arr){
        for(int16_t j = 1; j > -1; --j){
            register uint8_t half = (*arr >> (4*j)) & 0x0f;
            if(half < 10) usart_putchar(half + '0');
            else usart_putchar(half - 10 + 'a');
        }
        if(l % 16 == 15) usart_putchar('\n');
        else if(l & 1) usart_putchar(' ');
    }
}

#if USARTNUM == 2
void dma1_channel4_5_isr(){
    if(DMA1->ISR & DMA_ISR_TCIF4){ // Tx
        DMA1->IFCR |= DMA_IFCR_CTCIF4; // clear TC flag
        txrdy = 1;
    }
}
// USART1
#elif USARTNUM == 1
void dma1_channel2_3_isr(){
    if(DMA1->ISR & DMA_ISR_TCIF2){ // Tx
        DMA1->IFCR |= DMA_IFCR_CTCIF2; // clear TC flag
        txrdy = 1;
    }
}
#else
#error "Not implemented"
#endif
