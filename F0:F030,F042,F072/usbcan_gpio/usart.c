/*
 * This file is part of the usbcangpio project.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stm32f0.h>
#include <string.h>

#include "flash.h"
#include "ringbuffer.h"
#include "usart.h"

// We share DMAch4/5 for both USARTs, so only one can work in a time!!!

#define USARTSNO    2

// buffers for DMA or interrupt-driven data management
// here we use INDEX (usart number minus 1)
static uint8_t  inbuffer[DMARXBUFSZ];   // DMA in buffer
static uint8_t  rbbuffer[RXRBSZ];       // for in ringbuffer
static uint8_t  outbuffer[DMATXBUFSZ];  // DMA out buffer
static uint8_t  TXrdy = 1;              // TX DMA ready
static uint8_t  RXrdy = 0;              // == 1 when got IDLE or '\n' (only when `monitoring` is on
static uint8_t  textformat = 0;         // out by '\n'-terminated lines
static uint8_t  monitor = 0;            // monitor USART rx
static int      dma_read_idx = 0;       // start of data in DMA inbuffers
static int      curUSARTidx = -1;       // working USART index (0/1), -1 if unconfigured

static ringbuffer RBin  = {.data = rbbuffer, .length = RXRBSZ};

static volatile USART_TypeDef* const Usarts[USARTSNO] = {USART1, USART2};
static uint8_t const UIRQs[USARTSNO] = {USART1_IRQn, USART2_IRQn};

static usartconf_t usartconfig;
static uint8_t usartconfig_notinited = 1;
#define CHKUSARTCONFIG()  do{if(usartconfig_notinited) chkusartconf(NULL);}while(0)

// check config and if all OK, copy to local; if c == NULL, check local config and set defaults to wrong values
// return FALSE if some parameters was changed to default (in this case not copy to local)
int chkusartconf(usartconf_t *c){
    int ret = TRUE;
    if(usartconfig_notinited){
        usartconfig = the_conf.usartconfig;
        usartconfig_notinited = 0;
    }
    uint8_t copyto = TRUE;
    if(!c){
        copyto = FALSE;
        c = &usartconfig;
    }
    if(c->speed < USART_MIN_SPEED || c->speed > USART_MAX_SPEED){
        c->speed = USART_DEFAULT_SPEED;
        ret = FALSE;
    }
    // another tests could be here (like stopbits etc, if you wish)
    if(ret && copyto)  usartconfig = *c;
    return ret;
}

// just give default speed
void get_defusartconf(usartconf_t *c){
    if(!c) return;
    bzero(c, sizeof(usartconf_t));
    c->speed = USART_DEFAULT_SPEED;
}

int get_curusartconf(usartconf_t *c){
    CHKUSARTCONFIG();
    if(!c) return FALSE;
    *c = usartconfig;
    return TRUE;
}

/**
 * @brief usart_config - configure US[A]RT based on usb_LineCoding
 * @param usartconf (io) - (modified to real speeds); if NULL - get current
 * @return TRUE if all OK
 */
int usart_config(usartconf_t *uc){
    CHKUSARTCONFIG();
    if(uc && !chkusartconf(uc)) return FALSE;
    if(0 == usartconfig.RXen && 0 == usartconfig.TXen){ // no Rx/Tx
        usart_stop();
        return FALSE;
    }
    SYSCFG->CFGR1 |= SYSCFG_CFGR1_USART1RX_DMA_RMP | SYSCFG_CFGR1_USART1TX_DMA_RMP; // both USARTs on DMA1ch4(tx)/5(rx)
    // all clocking and GPIO config should be done in hardware_setup() & gpio_reinit()!
    if(curUSARTidx != -1) usart_stop(); // disable previous USART if enabled
    uint8_t No = usartconfig.idx;
    volatile USART_TypeDef *U = Usarts[No];
    uint32_t peripheral_clock = 48000000;
    // Disable USART while configuring
    U->CR1 = 0;
    U->ICR = 0xFFFFFFFF;   // Clear all interrupt flags
    // Assuming oversampling by 16 (default after reset). For higher baud rates you might use by 8.
    U->BRR = peripheral_clock / (usartconfig.speed);
    usartconfig.speed= peripheral_clock / U->BRR; // fix for real speed
    uint32_t cr1 = 0;
    // format: 8N1, so CR2 used only for character match (if need)
    if(usartconfig.monitor){
        if(usartconfig.textproto){
            U->CR2 = USART_CR2_ADD_VAL('\n');
            cr1 |= USART_CR1_CMIE;
        }else cr1 |= USART_CR1_IDLEIE; // monitor binary data by IDLE flag
    }
    textformat = usartconfig.textproto;
    monitor = usartconfig.monitor;
    // Enable transmitter, receiver, and interrupts (optional)
    if(usartconfig.RXen) cr1 |= USART_CR1_RE;
    if(usartconfig.TXen){
        cr1 |= USART_CR1_TE;
        // DMA Tx
        volatile DMA_Channel_TypeDef *T = DMA1_Channel4;
        T->CCR = 0;
        T->CPAR = (uint32_t) &U->TDR;
        T->CCR = DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE;
    }
    // Main config
    U->CR1 = cr1;
    curUSARTidx = No;
    // all OK -> copy to global config
    the_conf.usartconfig = usartconfig;
    return TRUE;
}

// start NUMBER
int usart_start(){
    if(curUSARTidx == -1) return FALSE;
    volatile USART_TypeDef *U = Usarts[curUSARTidx];
    if(monitor) NVIC_EnableIRQ(UIRQs[curUSARTidx]);
    NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);
    // reset Rx DMA
    if(U->CR1 & USART_CR1_RE){
        volatile DMA_Channel_TypeDef *R = DMA1_Channel5;
        dma_read_idx = 0;
        R->CCR = 0;
        R->CPAR = (uint32_t) U->RDR;
        R->CMAR = (uint32_t) inbuffer;
        R->CNDTR = DMARXBUFSZ;
        R->CCR = DMA_CCR_MINC | DMA_CCR_CIRC | DMA_CCR_EN;
        RB_clearbuf(&RBin);
    }
    U->CR1 |= USART_CR1_UE; // enable USARTx
    U->ICR = 0xFFFFFFFF;   // Clear flags
    // Wait for the idle frame to complete (optional)
    uint32_t tmout = 16000000;
    while(!(U->ISR & USART_ISR_TC)){
        if (--tmout == 0) break;
    }
    TXrdy = 1;
    return TRUE;
}

/**
 * @brief usart_stop - turn off U[S]ART for given interface
 * @param ifNo - interface number
 */
void usart_stop(){
    if(curUSARTidx == -1) return;
    Usarts[curUSARTidx]->CR1 &= ~USART_CR1_UE;
    NVIC_DisableIRQ(DMA1_Channel4_5_IRQn);
    NVIC_DisableIRQ(UIRQs[curUSARTidx]);
    curUSARTidx = -1;
}

/**
 * @brief usart_receive - receive data from USART by user request (return none if monitor == 1)
 * @param buf (io) - user buffer
 * @param len - `buf` length
 * @return -1 if USART not configured or amount of data from ringbuffer
 */
int usart_receive(uint8_t *buf, int len){
    if(curUSARTidx == -1) return -1;
    if(textformat){
        int toN = RB_datalento(&RBin, '\n');
        if(toN == 0) return 0;
        if(toN < len) len = toN; // read only until '\n' in text format
    }
    int got = RB_read(&RBin, buf, len);
    if(got < 0) got = 0;
    return got;
}

/**
 * @brief usart_process - send/receive processing
 * Try to send data from output ringbuffer, check input DMA buffer and full input ringbuffer
 */
/**
 * @brief usart_process - USART data processing
 * @param buf - buffer for "async" messages (when monitor==1)
 * @param len - length of `buf`
 * @return amount of bytes read or -1 if USART isn't active
 */
int usart_process(uint8_t *buf, int len){
    if(curUSARTidx == -1 || !(Usarts[curUSARTidx]->CR1 & USART_CR1_UE)) return -1; // none activated or started
    int ret = 0; // returned value
    // Input data
    int write_idx = DMARXBUFSZ - DMA1_Channel5->CNDTR; // next symbol to be written
    int available = (write_idx - dma_read_idx); // length of data available
    if(available < 0) available += DMARXBUFSZ; // write to the left of read
    if(available){
        if(RXrdy){
            if(buf && len > 0){
                if(len < available) available = len;
            }else RXrdy = 0;
        }
        // TODO: force copying data to "async" buffer in case of overflow danger
        if(available >= (DMARXBUFSZ/2) || RXrdy){ // enough data or lonely couple of bytes but need to show
            // copy data in one or two chunks (wrap handling)
            int wrOK = FALSE;
            if(dma_read_idx + available <= DMARXBUFSZ){ // head before tail
                if(RXrdy){
                    memcpy(buf, &inbuffer[dma_read_idx], available);
                    ret = available;
                    wrOK = TRUE;
                }else{
                    if(available == RB_write(&RBin, &inbuffer[dma_read_idx], available)) wrOK = TRUE;
                }
            }else{ // head after tail - two chunks
                int first = DMARXBUFSZ - dma_read_idx;
                if(RXrdy){
                    memcpy(buf, &inbuffer[dma_read_idx], first);
                    memcpy(buf, inbuffer, available - first);
                    ret = available;
                    wrOK = TRUE;
                }else{
                    if((first == RB_write(&RBin, &inbuffer[dma_read_idx], first)) &&
                        (available - first) == RB_write(&RBin, inbuffer, available - first)) wrOK = TRUE;
                }
            }
            if(wrOK){
                RXrdy = 0;
                dma_read_idx = write_idx; // update read pointer
            }
        }
    }
#if 0
    // Output data
    if(TXrdy){ // ready to send new data - here we can process RBout, if have
        int got = RB_read...
        if(got > 0){ // send next data portion
            volatile DMA_Channel_TypeDef *T = cfg->dma_tx_channel;
            T->CCR &= ~DMA_CCR_EN;
            T->CMAR = (uint32_t) outbuffers[i];
            T->CNDTR = got;
            TXrdy = 0;
            T->CCR |= DMA_CCR_EN; // start new transmission
        }
    }
#endif
    return ret;
}

// send data buffer
int usart_send(const uint8_t *data, int len){
    if(curUSARTidx == -1 || !data || len < 1) return 0;
    if(TXrdy == 0) return -1;
    if(len > DMATXBUFSZ) len = DMATXBUFSZ;
    memcpy(outbuffer, data, len);
    volatile DMA_Channel_TypeDef *T = DMA1_Channel4;
    T->CCR &= ~DMA_CCR_EN;
    T->CMAR = (uint32_t) outbuffer;
    T->CNDTR = len;
    TXrdy = 0;
    T->CCR |= DMA_CCR_EN; // start new transmission
    return len;
}

/**
 * @brief usart_isr - U[S]ART interrupt: IDLE (for DMA-driven) or
 * @param ifno - interface index
 */
static void usart_isr(){
    if(curUSARTidx == -1) return; // WTF???
    volatile USART_TypeDef *U = Usarts[curUSARTidx];
    // IDLE active when we monitor binary data
    if((U->ISR & USART_ISR_IDLE) && (U->CR1 & USART_CR1_IDLEIE)){ // try to send collected data (DMA-driven)
        RXrdy = 1; // seems like data portion is over - try to send it
    }
    if((U->ISR & USART_ISR_CMF) && (U->CR1 & USART_CR1_CMIE)){ // character match -> the same for text data
        RXrdy = 1;
    }
    U->ICR = 0xffffffff; // clear all flags
}

void dma1_channel4_5_isr(){ // TX ready, channel5
    if(DMA1->ISR & DMA_ISR_TCIF5){
        TXrdy = 1;
        DMA1->IFCR = DMA_IFCR_CTCIF5;
        DMA1_Channel4->CCR &= ~DMA_CCR_EN; // disable DMA channel until next send
    }
}

// U[S]ART interrupts
void usart1_isr() {usart_isr();}
void usart2_isr() {usart_isr();}
