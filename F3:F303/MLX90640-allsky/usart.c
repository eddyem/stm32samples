/*
 * This file is part of the ir-allsky project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stm32f3.h>
#include <string.h>

#include "hardware.h"
#include "ringbuffer.h"
#include "usart.h"

// unlike USB where you can hold NACK until user process frees ringbuffer, here we can't do that
// so USART writing forced by user (ringbuffer full or timeout by `usart_chk`)

extern volatile uint32_t Tms;
// flags
static volatile uint8_t bufovr = 0, // input buffer overfull
                        rbufno = 0, // index of active receiving buffer
                        txrdy  = 1; // transmission done

static char rbuf[2][UARTBUFSZI]; // double receiving buffer
static char *recvdata = NULL; // pointer to last received data
static volatile int recvdatalen = 0;
// to transmit images we reserve circular buffer large enough to hold full image
static uint8_t rbdata[DMARBSZ];
static ringbuffer dmarb = {.data = rbdata, .length = DMARBSZ, .head = 0, .tail = 0};

static int transmit_tbuf();

// return 1 if overflow was
int usart_ovr(){
    if(bufovr){
        bufovr = 0;
        return 1;
    }
    return 0;
}

// check if the buffer was filled >TRANSMIT_DELAY ago (transmit it then)
void usart_process(){
    transmit_tbuf();
}

/**
 * @brief usart_getline - read one dataportion
 * @param buf - user buffer
 * @param len - its length
 * @return amount of bytes
 */
char *usart_getline(int *len){
    if(!recvdatalen) return NULL;
    if(len) *len = recvdatalen;
    recvdatalen = 0;
    return recvdata;
}

// transmit next dataportion from ringbuffer
static int transmit_tbuf(){
    static uint8_t tbuf[UARTBUFSZO];
    uint32_t T0 = Tms;
    while(!txrdy && Tms - T0 < RXRDY_TMOUT) IWDG->KR = IWDG_REFRESH;
    if(!txrdy) return 0;
    int l = RB_read(&dmarb, tbuf, UARTBUFSZO);
    if(l < 1) return 1;
    txrdy = 0;
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;
    DMA1_Channel4->CMAR = (uint32_t) tbuf;
    DMA1_Channel4->CNDTR = l;
    DMA1_Channel4->CCR |= DMA_CCR_EN;
    return 1;
}

// return 0 if can't write to ringbuffer
int usart_putchar(const char ch){
    int r = RB_write(&dmarb, (uint8_t*)&ch, 1);
    if(r != 1){
        if(transmit_tbuf()) r = RB_write(&dmarb, (uint8_t*)&ch, 1);
    }
    return r;
}

// @return amount of written bytes
int usart_send(const uint8_t *data, int len){
    if(len > DMARBSZ) return FALSE;
    int L = 0;
    do{
        int r = RB_write(&dmarb, data, len - L);
        if(r < 1){
            if(!transmit_tbuf()) return L;
            else continue;
        }
        L += r;
        data += r;
    }while(L < len);
    return L;
}

// WARNING! strlen of `str` should be less than RBout size!
// @return amount of written bytes
int usart_sendstr(const char *str){
    return usart_send((uint8_t*)str, strlen(str));
}

// USART1: Rx - PA10 (AF7), Tx - PA9  (AF7)
int usart_setup(uint32_t speed){
    if(speed < 200 || speed > 3000000) return FALSE;
    // setup pins:
    GPIOA->MODER = (GPIOA->MODER & (MODER_CLR(9) & MODER_CLR(10))) |
                    MODER_AF(9) | MODER_AF(10);
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(GPIO_AFRH_AFRH1 | GPIO_AFRH_AFRH2)) |
                    AFRf(7, 9) | AFRf(7, 10);
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
    USART1->BRR = SysFreq / speed;
    USART1->CR3 = USART_CR3_DMAT | USART_CR3_DMAR; // enable DMA Tx/Rx
    USART1->CR2 = USART_CR2_ADD_VAL('\n'); // init character match register: our input proto is string-based
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_CMIE; // 1start,8data,nstop; enable Rx,Tx,USART; enable CharacterMatch Irq
    uint32_t tmout = 16000000;
    while(!(USART1->ISR & USART_ISR_TC)){if(--tmout == 0) break;} // polling idle frame Transmission
    USART1->ICR = 0xffffffff; // clear all flags again
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    NVIC_SetPriority(DMA1_Channel5_IRQn, 0);
    NVIC_SetPriority(USART1_IRQn, 4); // set character match priority lower
    return TRUE;
}

void usart_stop(){
    RCC->APB2ENR &= ~RCC_APB2ENR_USART1EN;
}

// USART1 character match interrupt
void usart1_exti25_isr(){
    DMA1_Channel5->CCR &= ~DMA_CCR_EN; // temporaly disable DMA
    USART1->ICR = USART_ICR_CMCF; // clear character match flag
    register int l = UARTBUFSZI - DMA1_Channel5->CNDTR - 1; // substitute '\n' with '\0', omit empty strings!
    if(l > 0){
        if(recvdata){ // user didn't read old data - mark as buffer overflow
            bufovr = 1;
        }
        recvdata = rbuf[rbufno];
        recvdata[l] = 0;
        rbufno = !rbufno;
        recvdatalen = l;
    }
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
