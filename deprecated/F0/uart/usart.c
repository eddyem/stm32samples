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
#include "usart.h"
#include <string.h>

int linerdy = 0,        // received data line length
    bufovr = 0,         // input buffer overfull
    txrdy = 1           // transmission done
;


int rbufno = 0; // current rbuf number
static char rbuf[UARTBUFSZ][2], tbuf[UARTBUFSZ]; // receive & transmit buffers
static char *recvdata = NULL;

/**
 * return length of received data (without trailing zero
 */
int usart2_getline(char **line){
    if(bufovr){
        bufovr = 0;
        linerdy = 0;
        return 0;
    }
    int L = linerdy;
    *line = recvdata;
    linerdy = 0;
    return L;
}

TXstatus usart2_send(const char *str, int len){
    if(!txrdy) return LINE_BUSY;
    if(len > UARTBUFSZ) return STR_TOO_LONG;
    txrdy = 0;
    bufovr = 0;
    memcpy(tbuf, str, len);
    DMA1_CCR4 &= ~DMA_CCR_EN;
    DMA1_CNDTR4 = len;
    DMA1_CCR4 |= DMA_CCR_EN; // start transmission
    return ALL_OK;
}

TXstatus usart2_send_blocking(const char *str, int len){
    if(!txrdy) return LINE_BUSY;
    int i;
    bufovr = 0;
    for(i = 0; i < len; ++i){
        USART2_TDR = *str++;
        while(!(USART2_ISR & USART_ISR_TXE));
    }
    txrdy = 1;
    return ALL_OK;
}


// Nucleo's USART2 connected to VCP proxy of st-link
void usart2_setup(){
    // setup pins: PA2 (Tx - AF1), PA15 (Rx - AF1)
    RCC_AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_DMAEN;
    //RCC_APB2ENR |= RCC_APB2ENR_SYSCFGEN; // turn on syscfg
    // AF mode
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2|GPIO15);
    gpio_set_af(GPIOA, GPIO_AF1, GPIO2|GPIO15);
    // DMA: Tx - Ch4
    DMA1_CPAR4 = (uint32_t) &USART2_TDR; // periph
    DMA1_CMAR4 = (uint32_t) tbuf; // mem
    DMA1_CCR4 |= DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE; // 8bit, mem++, mem->per, transcompl irq
    // Tx CNDTR set @ each transmission due to data size
    // DMA: Rx - Ch5
    DMA1_CPAR5 = (uint32_t) &USART2_RDR;
    DMA1_CMAR5 = (uint32_t) rbuf[rbufno];
    DMA1_CNDTR5 = UARTBUFSZ;
    DMA1_CCR5 |= DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_EN; // 8bit, mem++, per->mem, transcompl irq, enable receiver
    nvic_set_priority(NVIC_DMA1_CHANNEL4_5_IRQ, 3);
    nvic_enable_irq(NVIC_DMA1_CHANNEL4_5_IRQ);
    nvic_set_priority(NVIC_USART2_IRQ, 0);
    // setup usart2
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN; // clock
    USART2_CR2 = USART_CR2_ADD_VAL('\n'); // set newline as 'character match' interrupt
    // oversampling by16, 115200bps (fck=48mHz)
    //USART2_BRR = 0x1a1; // 48000000 / 115200
    USART2_BRR = 480000 / 1152;
    USART2_CR3 = USART_CR3_DMAR | USART_CR3_DMAT; // enable DMA Tx/Rx
    while(!(USART2_ISR & USART_ISR_TC)); // polling idle frame Transmission
    USART2_ICR |= USART_ICR_TCCF; // clear TC flag
    USART2_CR1 = USART_CR1_CMIE | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; // 1start,8data,nstop; enable Rx,Tx,USART, CM interrupt
    nvic_enable_irq(NVIC_USART2_IRQ);
}


void dma1_channel4_5_isr(){
    if(DMA1_ISR & DMA_ISR_TCIF4){ // Tx
        DMA1_IFCR |= DMA_IFCR_CIF4; // clear TC flag
        txrdy = 1;
    }
    if(DMA1_ISR & DMA_ISR_TCIF5){ // Rx
        DMA1_IFCR |= DMA_IFCR_CIF5; // clear TC flag
        DMA1_CCR5 &= ~DMA_CCR_EN; // turn off Rx DMA to refresh CNDTR
        DMA1_CNDTR5 = UARTBUFSZ;
        DMA1_CCR5 |= DMA_CCR_EN;
        bufovr = 1; // set buffer overrun flag: Rx buffer full, but no '\n' met!
    }
}

void usart2_isr(){
    if(USART2_ISR & USART_ISR_CMF){ // character match
        USART2_ICR |= USART_ISR_CMF; // clear flag
        linerdy = UARTBUFSZ - DMA1_CNDTR5 - 1; // set 'line ready' flag to data lenght
        recvdata = rbuf[rbufno];
        recvdata[linerdy] = 0;
        rbufno = !rbufno;
        // reload DMA Rx
        DMA1_CCR5 &= ~DMA_CCR_EN;
        DMA1_CMAR5 = (uint32_t) rbuf[rbufno];
        DMA1_CNDTR5 = UARTBUFSZ;
        DMA1_CCR5 |= DMA_CCR_EN;
    }
}
