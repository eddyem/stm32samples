/*
 * This file is part of the as3935 project.
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
 * along with this program.  If not, see <http://gnu.org>.
 */

#include <stm32f1.h>
#include "hardware.h"
#include "usart.h"

static volatile int idatalen[2] = {0, 0}; // received data line length (without '\n')
static volatile int odatalen[2] = {0, 0};

static volatile int usart_txrdy = 1;    // DMA transmission done
static volatile int usart_linerdy = 0;  // received data ready
static volatile int dlen = 0;           // length of data in current buffer
static volatile int usart_bufovr = 0;   // input buffer overfull

// Rx time and Rx progress flag
static volatile uint32_t last_rx_time = 0;
static volatile uint8_t rx_in_progress = 0;

static int rbufno = 0, tbufno = 0; // current rbuf/tbuf numbers
static char rbuf[2][UARTBUFSZI], tbuf[2][UARTBUFSZO]; // receive & transmit buffers
static char* volatile recvdata = NULL;

/**
 * return length of received data (without trailing zero) or -1 if overflow
 */
int usart_getline(char **line){
    if(usart_bufovr){
        usart_bufovr = 0;
        usart_linerdy = 0;
        return -1;
    }
    if(!usart_linerdy) return 0;
    *line = recvdata;
    usart_linerdy = 0;
    int x = dlen;
    dlen = 0;
    return x;
}

// transmit current tbuf and swap buffers
int usart_transmit(){
    register int l = odatalen[tbufno];
    if(!l) return 0;
    uint32_t tmout = 18000000;
    while(!usart_txrdy){
        IWDG->KR = IWDG_REFRESH;
        if(--tmout == 0) return 0;
    } // wait for previous DMA buffer transmission
    usart_txrdy = 0;
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;
    DMA1_Channel4->CMAR = (uint32_t) tbuf[tbufno];
    DMA1_Channel4->CNDTR = l;
    RS485_TX();
    DMA1_Channel4->CCR |= DMA_CCR_EN;
    tbufno = !tbufno;
    odatalen[tbufno] = 0;
    return l;
}

int usart_putchar(const char ch){
    if(odatalen[tbufno] == UARTBUFSZO){
        if(!usart_transmit()) return 0;
    }
    tbuf[tbufno][odatalen[tbufno]++] = ch;
    return 1;
}

int usart_send(const char *str){
    if(!str) return 0;
    int l = 0;
    while(*str){
        IWDG->KR = IWDG_REFRESH;
        if(odatalen[tbufno] == UARTBUFSZO){
            if(!usart_transmit()) return 0;
        }
        tbuf[tbufno][odatalen[tbufno]++] = *str++;
        ++l;
    }
    return l;
}

void usart_setup(uint32_t speed){
    if(speed < 1200) speed = 115200;
    // PA9 - Tx, PA10 - Rx (configured in gpio_setup, ensure clocks enabled)
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    // USART1 Tx DMA - Channel4
    DMA1_Channel4->CPAR = (uint32_t) &USART1->DR;
    DMA1_Channel4->CCR |= DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE; // 8bit, mem++, mem->per, transcompl irq
    NVIC_SetPriority(DMA1_Channel4_IRQn, 3);
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    NVIC_SetPriority(USART1_IRQn, 0);
    // setup usart1
    USART1->BRR = 72000000 / speed;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE; // 1start, 8data, nstop; enable Rx/Tx
    uint32_t tmout = 16000000;
    while(!(USART1->SR & USART_SR_TC)){ // polling idle frame Transmission
        IWDG->KR = IWDG_REFRESH;
        if(--tmout == 0) break;
    }
    (void) USART1->DR; // clear IDLE/RXNE etc
    USART1->SR = 0; // clear flags
    USART1->CR3 = USART_CR3_DMAT; // enable DMA Tx
    USART1->CR1 |= USART_CR1_RXNEIE | USART_CR1_TCIE; // allow Rx and Transmission Complete IRQs
    RS485_RX();
    NVIC_EnableIRQ(USART1_IRQn);
}

// return 1 if there's some data in Tx buffer
int usart_has_txdata(){
    return odatalen[tbufno] > 0;
}

// return 1 if Rx isn't in progress
int usart_bus_free(){
    if(rx_in_progress){
        if(Tms - last_rx_time > 50){ // 50ms timeout of broken message
            rx_in_progress = 0;
            idatalen[rbufno] = 0;
        }else{
            return 0;
        }
    }
    if(usart_linerdy || !usart_txrdy) return 0;
    return (Tms - last_rx_time) >= 5; // 5ms pause of receiving data
}

void usart1_isr(){
    uint32_t sr = USART1->SR;
    if(sr & USART_SR_RXNE){ // RX not empty - receive next char
        uint8_t rb = USART1->DR;
        last_rx_time = Tms;
        if(rb == '\r') return; // ignore CR if CRLF is sent
        if(idatalen[rbufno] < UARTBUFSZI - 1){ // keep 1 byte for '\0'
            if(rb == '\n'){ // got newline - line ready
                rx_in_progress = 0;
                rbuf[rbufno][idatalen[rbufno]] = 0;
                usart_linerdy = 1;
                dlen = idatalen[rbufno];
                recvdata = rbuf[rbufno];
                // prepare other buffer
                rbufno = !rbufno;
                idatalen[rbufno] = 0;
            }else{
                rx_in_progress = 1;
                rbuf[rbufno][idatalen[rbufno]++] = rb;
            }
        }else{ // buffer overrun
            usart_bufovr = 1;
            idatalen[rbufno] = 0;
        }
    }
    if(sr & USART_SR_TC){ // last bit shifted out -> switch RS-485 back to RX
        USART1->SR &= ~USART_SR_TC;
        if(usart_txrdy){
            RS485_RX();
        }
    }
}

void dma1_channel4_isr(){
    if(DMA1->ISR & DMA_ISR_TCIF4){ // Tx DMA complete
        DMA1->IFCR = DMA_IFCR_CTCIF4;
        usart_txrdy = 1;
        // If USART already finished shifting out the last byte, switch to RX immediately
        if(USART1->SR & USART_SR_TC){
            USART1->SR &= ~USART_SR_TC;
            RS485_RX();
        }
    }
}

