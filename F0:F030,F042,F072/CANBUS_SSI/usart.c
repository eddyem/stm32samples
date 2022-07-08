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
#include "hardware.h"
#include "usart.h"

#include <stm32f0.h>
#include <string.h>

typedef enum{
    READING,
    SENDING,
    WAITING
} _485_state;
static _485_state st = READING; // RS-485 state: Rx, Tx, TX last byte

// switch to Rx/Tx:
#define _485_Rx()  do{RS485_RX(); st = READING; USARTX->CR1 = (USARTX->CR1 & ~USART_CR1_TE) | USART_CR1_RE;}while(0)
#define _485_Tx()  do{RS485_TX(); st = SENDING; USARTX->CR1 = (USARTX->CR1 & ~USART_CR1_RE) | USART_CR1_TE;}while(0)

static int datalen[2] = {0,0}; // received data line length (including '\n')

volatile int
    linerdy = 0,    // received data ready
    bufovr = 0      // input buffer overfull
;

static volatile int
    dlen = 0,       // length of data (including '\n') in current buffer
    txrdy = 1       // transmission done
;

static int rbufno = 0; // current rbuf number
static char rbuf[UARTBUFSZ][2]; // receive buffers
static char *recvdata = NULL;

/**
 * return length of received data (without trailing zero)
 */
int usart_getline(char **line){
    if(!line) return 0;
    if(bufovr){
        bufovr = 0;
        linerdy = 0;
        return 0;
    }
    *line = recvdata;
    line[dlen] = 0;
    linerdy = 0;
    return dlen;
}

/**
 * @brief usart_send_blocking - blocking send of any message
 * @param str - message to send
 * @param len - its length
 */
void usart_send_blocking(const char *str, int len){
    uint32_t tmout = 160000;
    while(!txrdy){
        IWDG->KR = IWDG_REFRESH;
        if(--tmout == 0) return;
    }
    _485_Tx();
    bufovr = 0;
    for(int i = 0; i < len; ++i){
        USARTX -> TDR = *str++;
        while(!(USARTX->ISR & USART_ISR_TXE)){IWDG->KR = IWDG_REFRESH;}
    }
    // wait for transfer complete to switch into Rx
    while(!(USARTX->ISR & USART_ISR_TC)){IWDG->KR = IWDG_REFRESH;}
    _485_Rx();
}

void usart_setup(){
#if USARTNUM == 2
    // setup pins: PA2 (Tx - AF1), PA15 (Rx - AF1)
    // AF mode (AF1)
    GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER2|GPIO_MODER_MODER15))\
                | (GPIO_MODER_MODER2_AF | GPIO_MODER_MODER15_AF);
    GPIOA->AFR[0] = (GPIOA->AFR[0] &~GPIO_AFRH_AFRH2) | 1 << (2 * 4); // PA2
    GPIOA->AFR[1] = (GPIOA->AFR[1] &~GPIO_AFRH_AFRH7) | 1 << (7 * 4); // PA15
    // setup usart2
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN; // clock
    // oversampling by16, 115200bps (fck=48mHz)
    //USART2_BRR = 0x1a1; // 48000000 / 115200
    USART2->BRR = 480000 / 1152;
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
    // Tx CNDTR set @ each transmission due to data size
    NVIC_SetPriority(USART1_IRQn, 0);
    // setup usart1
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    USART1->BRR = 480000 / 1152;
    USART1->CR1 = USART_CR1_UE; // 1start,8data,nstop; enable USART
    while(!(USART1->ISR & USART_ISR_TC)); // polling idle frame Transmission
    USART1->ICR |= USART_ICR_TCCF; // clear TC flag
    USART1->CR1 |= USART_CR1_RXNEIE;
    NVIC_EnableIRQ(USART1_IRQn);
#else
#error "Not implemented"
#endif
    _485_Rx(); // turn RX on (enable Rx, disable Tx)
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
        if(datalen[rbufno] < UARTBUFSZ-1){ // put next char into buf
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
        }else{ // buffer overfull
            bufovr = 1;
            datalen[rbufno] = 0;
            #ifdef CHECK_TMOUT
            tmout = 0;
            #endif
        }
    }
}


/**
 * @brief usart_proc - switch 485 to Rx when all data received
 */
void usart_proc(){
    switch(st){
        case SENDING:
            if(txrdy) st = WAITING;
        break;
        case WAITING:
            if(USARTX->ISR & USART_ISR_TC){ // last byte done -> Rx
                _485_Rx();
            }
        break;
        default:
        break;
    }
}
