/*us
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
static int datalen[2] = {0,0}; // received data line length (including '\n')

volatile int linerdy = 0,        // received data ready
    dlen = 0,           // length of data (including '\n') in current buffer
    bufovr = 0,         // input buffer overfull
    txrdy = 1           // transmission done
;


int rbufno = 0; // current rbuf number
static char rbuf[UARTBUFSZ][2], tbuf[UARTBUFSZ]; // receive & transmit buffers
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

TXstatus usart_send(const char *str, int len){
    if(!txrdy) return LINE_BUSY;
    if(len > UARTBUFSZ) return STR_TOO_LONG;
    txrdy = 0;
    memcpy(tbuf, str, len);
#if USARTNUM == 2
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;
    DMA1_Channel4->CNDTR = len;
    DMA1_Channel4->CCR |= DMA_CCR_EN; // start transmission
#elif USARTNUM == 1
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    DMA1_Channel2->CNDTR = len;
    DMA1_Channel2->CCR |= DMA_CCR_EN;
#else
#error "Not implemented"
#endif
    return ALL_OK;
}

TXstatus usart_send_blocking(const char *str, int len){
    if(!txrdy) return LINE_BUSY;
    int i;
    bufovr = 0;
    for(i = 0; i < len; ++i){
        USARTX -> TDR = *str++;
        while(!(USARTX->ISR & USART_ISR_TXE));
    }
    return ALL_OK;
}

void usart_putchar(const char ch){
    while(!txrdy);
    USARTX -> TDR = ch;
    while(!(USARTX->ISR & USART_ISR_TXE));
}

void newline(){
    while(!txrdy);
    USARTX -> TDR = '\n';
    while(!(USARTX->ISR & USART_ISR_TXE));
}


void usart_setup(){
// Nucleo's USART2 connected to VCP proxy of st-link
#if USARTNUM == 2
    // setup pins: PA2 (Tx - AF1), PA15 (Rx - AF1)
    // AF mode (AF1)
    GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER2|GPIO_MODER_MODER15))\
                | (GPIO_MODER_MODER2_AF | GPIO_MODER_MODER15_AF);
    GPIOA->AFR[0] = (GPIOA->AFR[0] &~GPIO_AFRH_AFRH2) | 1 << (2 * 4); // PA2
    GPIOA->AFR[1] = (GPIOA->AFR[1] &~GPIO_AFRH_AFRH7) | 1 << (7 * 4); // PA15
    // DMA: Tx - Ch4
    DMA1_Channel4->CPAR = (uint32_t) &USART2->TDR; // periph
    DMA1_Channel4->CMAR = (uint32_t) tbuf; // mem
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
    while(!(USART2->ISR & USART_ISR_TC)); // polling idle frame Transmission
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
    while(!(USART1->ISR & USART_ISR_TC)); // polling idle frame Transmission
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
            datalen[rbufno] = 0;
        }
        tmout = Tms + TIMEOUT_MS;
        if(!tmout) tmout = 1; // prevent 0
        #endif
        // read RDR clears flag
        uint8_t rb = USARTX->RDR;
        if(datalen[rbufno] < UARTBUFSZ){ // put next char into buf
            rbuf[rbufno][datalen[rbufno]++] = rb;
            if(rb == '\n'){ // got newline - line ready
                linerdy = 1;
                dlen = datalen[rbufno];
                recvdata = rbuf[rbufno];
                // prepare other buffer
                rbufno = !rbufno;
                datalen[rbufno] = 0;
                #ifdef CHECK_TMOUT
                // clear timeout at line end
                tmout = 0;
                #endif
            }
        }else{ // buffer overrun
            bufovr = 1;
            datalen[rbufno] = 0;
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
    while(LINE_BUSY == usart_send_blocking(bufa, l+bpos));
}

// print 32bit unsigned int as hex
void printuhex(uint32_t val){
    SEND("0x");
    uint8_t *ptr = (uint8_t*)&val + 3;
    int i, j;
    for(i = 0; i < 4; ++i, --ptr){
        for(j = 1; j > -1; --j){
            uint8_t half = (*ptr >> (4*j)) & 0x0f;
            if(half < 10) usart_putchar(half + '0');
            else usart_putchar(half - 10 + 'a');
        }
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
