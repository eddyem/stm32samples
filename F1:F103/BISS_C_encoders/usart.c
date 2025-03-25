/*
 * usart.c
 *
 * Copyright 2018 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include <string.h>

#include "stm32f1.h"
#include "usart.h"
#include "ringbuffer.h"

extern volatile uint32_t Tms;
// buffers for out ringbuffer and DMA send
static uint8_t buf[UARTBUFSZ], txbuf[UARTBUFSZ];
static ringbuffer ringbuf = {.data = buf, .length = UARTBUFSZ};

static volatile int usart_txrdy = 1; // transmission done

// transmit current tbuf
void usart_transmit(){
    if(RB_hasbyte(&ringbuf, '\n') < 0 || !usart_txrdy) return;
    int L = 0, l = 0;
    do{
        l = RB_readto(&ringbuf, '\n', txbuf + L, UARTBUFSZ - L);
        if(l > 0) L += l;
    }while(l > 0 && L < UARTBUFSZ);
    if(L < 1) return;
    usart_txrdy = 0;
    if(L < UARTBUFSZ-1){
        txbuf[L++] = '$'; txbuf[L++] = '\n';
    }
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;
    DMA1_Channel4->CMAR = (uint32_t) txbuf; // mem
    DMA1_Channel4->CNDTR = L;
    DMA1_Channel4->CCR |= DMA_CCR_EN;
}

void usart_putchar(const char ch){
    RB_write(&ringbuf, (const uint8_t*)&ch, 1);
}

void usart_send(const char *str){
    int l = strlen(str);
    if(RB_datalen(&ringbuf) > UARTBUFSZ/2) usart_transmit();
    RB_write(&ringbuf, (const uint8_t*)str, l);
}

/*
 * USART speed: baudrate = Fck/(USARTDIV)
 * USARTDIV stored in USART->BRR
 *
 * for 72MHz USARTDIV=72000/f(kboud); so for 115200 USARTDIV=72000/115.2=625 -> BRR=0x271
 *         9600: BRR = 7500 (0x1D4C)
 */

void usart_setup(){
    uint32_t tmout = 16000000;
    // PA9 - Tx, PA10 - Rx
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN | RCC_APB2ENR_AFIOEN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    GPIOA->CRH |= CRH(9, CNF_AFPP|MODE_NORMAL) | CRH(10, CNF_FLINPUT|MODE_INPUT);

    // USART1 Tx DMA - Channel4 (Rx - channel 5)
    DMA1_Channel4->CPAR = (uint32_t) &USART1->DR; // periph
    DMA1_Channel4->CCR |= DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE; // 8bit, mem++, mem->per, transcompl irq
    // Tx CNDTR set @ each transmission due to data size
    NVIC_SetPriority(DMA1_Channel4_IRQn, 3);
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    NVIC_SetPriority(USART1_IRQn, 0);
    // setup usart1
    USART1->BRR = 72000000 / 4000000;
    USART1->CR1 = USART_CR1_TE | USART_CR1_UE; // 1start,8data,nstop; enable Rx,Tx,USART
    while(!(USART1->SR & USART_SR_TC)){if(--tmout == 0) break;} // polling idle frame Transmission
    USART1->SR = 0; // clear flags
    USART1->CR1 |= USART_CR1_RXNEIE; // allow Rx IRQ
    USART1->CR3 = USART_CR3_DMAT; // enable DMA Tx
}

void dma1_channel4_isr(){
    if(DMA1->ISR & DMA_ISR_TCIF4){ // Tx
        DMA1->IFCR = DMA_IFCR_CTCIF4; // clear TC flag
        usart_txrdy = 1;
    }
}
