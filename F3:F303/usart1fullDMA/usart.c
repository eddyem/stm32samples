/*
 * This file is part of the F303usartDMA project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include <string.h>

extern volatile uint32_t Tms;
static uint32_t lastAccessT = 0; // Tms of last Tx access

// flags
static volatile uint8_t bufovr = 0, // input buffer overfull
                        rxrdy  = 0, // received data ready
                        txrdy  = 1; // transmission done

// length of data (including '\n') in current buffer
static volatile int dlen = 0;
static volatile int odatalen[2] = {0};

static uint8_t rbufno = 0, tbufno = 0; // current rbuf/tbuf numbers
static char rbuf[2][UARTBUFSZI], tbuf[2][UARTBUFSZO]; // receive & transmit buffers
static char *recvdata = NULL;

static int transmit_tbuf();

// return 1 if overflow was
int usart_ovr(){
    if(bufovr){
        bufovr = 0;
        return 1;
    }
    return 0;
}

// check if the buffer was filled >10ms ago (transmit it then)
// @return rxrdy flag
int chk_usart(){
    if(Tms - lastAccessT > TRANSMIT_DELAY){
        transmit_tbuf();
        lastAccessT = Tms;
    }
    int ret = rxrdy;
    rxrdy = 0;
    return ret;
}

/**
 * return length of received data (without trailing zero
 */
int usart_getline(char **line){
    *line = recvdata;
    rxrdy = 0;
    recvdata = NULL;
    bufovr = 0;
    return dlen;
}

// transmit current tbuf and swap buffers
static int transmit_tbuf(){
    uint32_t p = 1000000;
    while(!txrdy && --p);
    if(!txrdy) return 0;
    register int l = odatalen[tbufno];
    if(!l) return 1;
    txrdy = 0;
    odatalen[tbufno] = 0;
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;
    DMA1_Channel4->CMAR = (uint32_t) tbuf[tbufno]; // mem
    DMA1_Channel4->CNDTR = l;
    DMA1_Channel4->CCR |= DMA_CCR_EN;
    tbufno = !tbufno;
    return 1;
}

void usart_putchar(const char ch){
    if(odatalen[tbufno] == UARTBUFSZO)
        if(!transmit_tbuf()) return;
    tbuf[tbufno][odatalen[tbufno]++] = ch;
}

void usart_send(const char *str){
    while(*str){
        if(odatalen[tbufno] == UARTBUFSZO)
            if(!transmit_tbuf()) return;
        tbuf[tbufno][odatalen[tbufno]++] = *str++;
    }
}

void usart_sendn(const char *str, uint32_t L){
    for(uint32_t i = 0; i < L; ++i){
        if(odatalen[tbufno] == UARTBUFSZO)
            if(!transmit_tbuf()) return;
        tbuf[tbufno][odatalen[tbufno]++] = *str++;
    }
}

// USART1: Rx - PA10 (AF7), Tx - PA9  (AF7)
void usart_setup(){
    // setup pins:
    GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER9 | GPIO_MODER_MODER10)) |
                    GPIO_MODER_MODER9_AF | GPIO_MODER_MODER10_AF;
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(GPIO_AFRH_AFRH1 | GPIO_AFRH_AFRH2)) |
                7 << (1 * 4) | 7 << (2 * 4); // PA9, PA10
    // clock
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    USART1->ICR = 0xffffffff; // clear all flags
    // Tx DMA
    DMA1_Channel4->CCR = 0;
    DMA1_Channel4->CPAR = (uint32_t) &USART1->TDR; // periph
    DMA1_Channel4->CCR |= DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE; // 8bit, mem++, mem->per, transcompl irq
    // Rx DMA
    DMA1_Channel5->CCR = 0;
    DMA1_Channel5->CPAR = (uint32_t) &USART1->RDR; // periph
    DMA1_Channel5->CMAR = (uint32_t) rbuf[0];
    DMA1_Channel5->CNDTR = UARTBUFSZI;
    DMA1_Channel5->CCR |= DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_EN; // 8bit, mem++, per->mem, transcompl irq, enable
    // setup usart
    USART1->BRR = 720000 / 1152;
    USART1->CR3 = USART_CR3_DMAT | USART_CR3_DMAR; // enable DMA Tx/Rx
    USART1->CR2 = USART_CR2_ADD_VAL('\n'); // init character match register
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_CMIE; // 1start,8data,nstop; enable Rx,Tx,USART; enable CharacterMatch Irq
    uint32_t tmout = 16000000;
    while(!(USART1->ISR & USART_ISR_TC)){if(--tmout == 0) break;} // polling idle frame Transmission
    USART1->ICR = 0xffffffff; // clear all flags again
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    NVIC_SetPriority(DMA1_Channel5_IRQn, 0);
    NVIC_SetPriority(USART1_IRQn, 4); // set character match priority lower
}

void usart_stop(){
    RCC->APB2ENR &= ~RCC_APB2ENR_USART1EN;
}

// USART1 character match interrupt
void usart1_exti25_isr(){
    DMA1_Channel5->CCR &= ~DMA_CCR_EN; // temporaly disable DMA
    USART1->ICR = USART_ICR_CMCF; // clear character match flag
    recvdata = rbuf[rbufno];
    register int l = UARTBUFSZI - DMA1_Channel5->CNDTR - 1;
    if(l > 0){
        recvdata[l] = 0;
        dlen = l;
        rxrdy = 1;
    }
    rbufno = !rbufno;
    DMA1_Channel5->CMAR = (uint32_t) rbuf[rbufno];
    DMA1_Channel5->CNDTR = UARTBUFSZI;
    DMA1_Channel5->CCR |= DMA_CCR_EN;
}

// USART1 Tx complete
void dma1_channel4_isr(){
    DMA1->IFCR |= DMA_IFCR_CTCIF4;
    txrdy = 1;
}

// USART1 Rx buffer overrun
void dma1_channel5_isr(){
    DMA1_Channel5->CCR &= ~DMA_CCR_EN;
    DMA1->IFCR |= DMA_IFCR_CTCIF5;
    DMA1_Channel5->CMAR = (uint32_t) rbuf[rbufno];
    DMA1_Channel5->CNDTR = UARTBUFSZI;
    bufovr = 1;
    DMA1_Channel5->CCR |= DMA_CCR_EN;
}
