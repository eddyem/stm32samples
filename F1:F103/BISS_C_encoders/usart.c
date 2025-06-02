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

// magick starting sequence
#define ENC_MAGICK  (204)

// buffers for out ringbuffer and DMA send
static uint8_t txbuf[UARTBUFSZ];
static volatile int usart_txrdy = 1; // transmission done

void usart_send(const uint8_t *buf, int buflen){
    while(!usart_txrdy);
    if(buflen > UARTBUFSZ) buflen = UARTBUFSZ;
    memcpy(txbuf, buf, buflen);
    DMA1_Channel7->CCR &= ~DMA_CCR_EN;
    DMA1_Channel7->CMAR = (uint32_t) txbuf; // mem
    DMA1_Channel7->CNDTR = buflen;
    usart_txrdy = 0;
    DMA1_Channel7->CCR |= DMA_CCR_EN;
}

// encoders raw data
typedef struct __attribute__((packed)){
    uint8_t magick;
    uint32_t encY;
    uint32_t encX;
    uint8_t crc[4];
} enc_t;

void usart_send_enc(uint32_t encX, uint32_t encY){
    enc_t edata;
    uint8_t *databuf = (uint8_t*) &edata;
    uint32_t POS_SUM = 0;
    for(int i = 1; i < 9; ++i) POS_SUM += databuf[i];
    edata.crc[0] = POS_SUM >> 8;
    edata.crc[1] = ((0xFFFF - POS_SUM) & 0xFF) - edata.crc[0];
    edata.crc[2] = (0xFFFF - POS_SUM) >> 8;
    edata.crc[3] = 0;
    edata.magick = ENC_MAGICK;
    edata.encX = encX;
    edata.encY = encY;
    usart_send(databuf, sizeof(enc_t));
}

/*
 * USART2 speed: baudrate = Fck/(USARTDIV)
 * USARTDIV stored in USART->BRR
 *
 * for 36MHz USARTDIV=36000/f(kboud)
 */
void usart_setup(){
    uint32_t tmout = 16000000;
    // PA9 - Tx, PA10 - Rx
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    GPIOA->CRL = (GPIOA->CRL & ~(GPIO_CRL_CNF2 | GPIO_CRL_CNF3)) |
        CRL(2, CNF_AFPP|MODE_NORMAL) | CRL(3, CNF_FLINPUT|MODE_INPUT);

    // USART1 Tx DMA - Channel4 (Rx - channel 5)
    DMA1_Channel7->CPAR = (uint32_t) &USART2->DR; // periph
    DMA1_Channel7->CCR |= DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE; // 8bit, mem++, mem->per, transcompl irq
    // Tx CNDTR set @ each transmission due to data size
    NVIC_SetPriority(DMA1_Channel7_IRQn, 3);
    NVIC_EnableIRQ(DMA1_Channel7_IRQn);
    //NVIC_SetPriority(USART1_IRQn, 0);
    // setup usart1
    USART2->BRR = 36000000 / 153000;
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE; // 1start,8data,nstop; enable Rx,Tx,USART
    while(!(USART1->SR & USART_SR_TC)){
        IWDG->KR = IWDG_REFRESH;
        if(--tmout == 0) break;
    } // polling idle frame Transmission
    USART2->SR = 0; // clear flags
    USART2->CR1 |= USART_CR1_RXNEIE; // allow Rx IRQ
    USART2->CR3 = USART_CR3_DMAT; // enable DMA Tx
}

void dma1_channel7_isr(){
    if(DMA1->ISR & DMA_ISR_TCIF7){ // Tx
        DMA1->IFCR = DMA_IFCR_CTCIF7; // clear TC flag
        usart_txrdy = 1;
    }
}
