/*
 * usart.c
 *
 * Copyright 2017 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include "usart.h"
#include <string.h> // memcpy

extern volatile uint32_t Tms;

static int datalen[2] = {0,0}; // received data line length (including '\n')

int linerdy = 0,        // received data ready
    dlen = 0,           // length of data (including '\n') in current buffer
    bufovr = 0,         // input buffer overfull
    txrdy = 1           // transmission done
;

static int  rbufno = 0; // current rbuf number
static char rbuf[2][UARTBUFSZ], tbuf[UARTBUFSZ]; // receive & transmit buffers
static char *recvdata = NULL;

void USART1_config(){
    /* Enable the peripheral clock of GPIOA */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    /* GPIO configuration for USART1 signals */
    /* (1) Select AF mode (10) on PA9 and PA10 */
    /* (2) AF1 for USART1 signals */
    GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER9|GPIO_MODER_MODER10))\
                | (GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1); /* (1) */
    GPIOA->AFR[1] = (GPIOA->AFR[1] &~ (GPIO_AFRH_AFRH1 | GPIO_AFRH_AFRH2))\
                | (1 << (1 * 4)) | (1 << (2 * 4)); /* (2) */
    /* Enable the peripheral clock USART1 */
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    /* Configure USART1 */
    /* (1) oversampling by 16, 115200 baud */
    /* (2) 8 data bit, 1 start bit, 1 stop bit, no parity */
    USART1->BRR = 480000 / 1152; /* (1) */
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; /* (2) */
    /* polling idle frame Transmission */
    while(!(USART1->ISR & USART_ISR_TC)){}
    USART1->ICR |= USART_ICR_TCCF; /* clear TC flag */
    USART1->CR1 |= USART_CR1_RXNEIE; /* enable TC, TXE & RXNE interrupt */
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel2->CPAR = (uint32_t) &(USART1->TDR); // periph
    DMA1_Channel2->CMAR = (uint32_t) tbuf; // mem
    DMA1_Channel2->CCR |= DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE; // 8bit, mem++, mem->per, transcompl irq
    USART1->CR3 = USART_CR3_DMAT;
    NVIC_SetPriority(DMA1_Channel2_3_IRQn, 3);
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    /* Configure IT */
    /* (3) Set priority for USART1_IRQn */
    /* (4) Enable USART1_IRQn */
    NVIC_SetPriority(USART1_IRQn, 0); /* (3) */
    NVIC_EnableIRQ(USART1_IRQn); /* (4) */
}

void usart1_isr(){
    #ifdef CHECK_TMOUT
    static uint32_t tmout = 0;
    #endif
    if(USART1->ISR & USART_ISR_RXNE){ // RX not emty - receive next char
        #ifdef CHECK_TMOUT
        if(tmout && Tms >= tmout){ // set overflow flag
            bufovr = 1;
            datalen[rbufno] = 0;
        }
        tmout = Tms + TIMEOUT_MS;
        if(!tmout) tmout = 1; // prevent 0
        #endif
        // read RDR clears flag
        uint8_t rb = USART1->RDR;
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

void dma1_channel2_3_isr(){
    if(DMA1->ISR & DMA_ISR_TCIF2){ // Tx
        DMA1->IFCR |= DMA_IFCR_CTCIF2; // clear TC flag
        txrdy = 1;
    }
}

/**
 * return length of received data (without trailing zero
 */
int usart1_getline(char **line){
    if(bufovr){
        bufovr = 0;
        linerdy = 0;
        return 0;
    }
    *line = recvdata;
    linerdy = 0;
    return dlen;
}

TXstatus usart1_send(const char *str, int len){
    if(!txrdy) return LINE_BUSY;
    if(len > UARTBUFSZ) return STR_TOO_LONG;
    txrdy = 0;
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    memcpy(tbuf, str, len);
    DMA1_Channel2->CNDTR = len;
    DMA1_Channel2->CCR |= DMA_CCR_EN; // start transmission
    return ALL_OK;
}

TXstatus usart1_send_blocking(const char *str, int len){
    if(!txrdy) return LINE_BUSY;
    int i;
    for(i = 0; i < len; ++i){
        USART1->TDR = *str++;
        while(!(USART1->ISR & USART_ISR_TXE));
    }
    txrdy = 1;
    return ALL_OK;
}
