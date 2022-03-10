/*
 * This file is part of the pl2303 project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

static volatile int idatalen = 0; // received data line length (including '\n')

volatile int linerdy = 0,   // received data ready
    dlen    = 0,            // length of data (including '\n') in current buffer
    bufovr  = 0;            // input buffer overfull

static volatile int rbufno = 0; // current rbuf number
static char rbuf[2][UARTBUFSZ]; // receive buffers
static volatile char *recvdata = NULL;

/**
 * return length of received data (without trailing zero)
 */
int usart_getline(char **line){
    if(bufovr){
        bufovr = 0;
        linerdy = 0;
        return 0;
    }
    if(!linerdy) return 0;
    *line = (char*)recvdata;
    linerdy = 0;
    return dlen;
}

void usart_putchar(const char ch){
    while(!(USART1->ISR & USART_ISR_TXE));
    USART1->TDR = ch;
}
void usart_sendn(const char *str, int L){
    if(!str) return;
    for(int i = 0; i < L; ++i){
        usart_putchar(str[i]);
    }
}
void usart_send(const char *str){
    if(!str) return;
    int L = 0;
    const char *ptr = str;
    while(*ptr++) ++L;
    usart_sendn(str, L);
}

void usart_setup(){
    // USART1: Rx - PA10 (AF7), Tx - PA9  (AF7)
    // setup pins:
    GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER9 | GPIO_MODER_MODER10)) |
                   GPIO_MODER_MODER9_AF | GPIO_MODER_MODER10_AF;
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(GPIO_AFRH_AFRH1 | GPIO_AFRH_AFRH2)) |
                7 << (1 * 4) | 7 << (2 * 4); // PA9, PA10
    // clock
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    USART1->ICR = 0xffffffff; // clear all flags
    USART1->BRR = SysFreq / 115200;
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_RXNEIE; // 1start,8data,nstop; enable Rx,Tx,USART
    uint32_t tmout = 16000000;
    while(!(USART1->ISR & USART_ISR_TC)){if(--tmout == 0) break;} // polling idle frame Transmission
    USART1->ICR = 0xffffffff; // clear all flags again
    NVIC_SetPriority(USART1_IRQn, 0);
    NVIC_EnableIRQ(USART1_IRQn);
}

void usart1_exti25_isr(){
    if(USART1->ISR & USART_ISR_RXNE){ // RX not emty - receive next char
        // read RDR clears flag
        uint8_t rb = USART1->RDR;
        if(idatalen < UARTBUFSZ){ // put next char into buf
            rbuf[rbufno][idatalen++] = rb;
            if(rb == '\n'){ // got newline - line ready
                linerdy = 1;
                dlen = idatalen;
                recvdata = rbuf[rbufno];
                recvdata[dlen-1] = 0;
                // prepare other buffer
                rbufno = !rbufno;
                idatalen = 0;
            }
        }else{ // buffer overrun
            bufovr = 1;
            idatalen = 0;
        }
    }
}
