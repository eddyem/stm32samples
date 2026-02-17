/*
 * This file is part of the MultiInterface project.
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

#include <stm32f3.h>
#include <string.h>

#include "Debug.h"
#include "hardware.h"
#include "strfunc.h"
#include "usart.h"
#include "usb_descr.h" // InterfacesAmount, IFx, bufsz
#include "usb_dev.h" // get fresh USB input data

// !!!!! INDEXED BY INTERFACE NUMBER (just to not check IF6 and IF7) !!!!!
#define USARTSNO    5

typedef struct {
    volatile USART_TypeDef *instance;               // U[S]ARTx
    uint32_t pclk_freq;                             // APB1/APB2 frequency
    int16_t  UIRQn;                                 // USART IRQ number
    int16_t  DIRQn;                                 // DMA Tx IRQ number (for DMA-driven)
    volatile DMA_TypeDef *dma_controller;           // DMA1/DMA2 or NULL if not used
    volatile DMA_Channel_TypeDef *dma_rx_channel;   // e.g., DMA_Channel_5 or NULL if not used
    volatile DMA_Channel_TypeDef *dma_tx_channel;   // e.g., DMA_Channel_4 or NULL if not used
    uint32_t TTCflag;                               // Tx transfer complete flag
    volatile GPIO_TypeDef *DEport;                  // if RS485 - DE GPIO port (NULL for RS-232 or RS-422)
    uint32_t DEpin;                                 // -//- pin
} USART_Config;

//   IF    U[S]ART bus  freq    TxDMA    RxDMA   DE (if 485)
// maybe DMA1ch2 would be for SPI1Rx (or SSI should be @ SPI3), in this case USART3 would be interrupt-driven
// IF1[0]: USART3  APB2 72 MHz  DMA1ch2 DMA1ch3  PB14
// IF2[1]: USART1  APB1 36 MHz  DMA1ch4 DMA1ch5  PB0
// IF3[2]: USART2  APB2 72 MHz  DMA1ch7 DMA1ch6  PA1
// IF4[3]: UART4   APB2 72 MHz  DMA2ch5 DMA2ch3
// IF5[4]: UART5   APB2 72 MHz  -       -       - interrupt-driven
// IF6[5]: (CAN)
// IF7[6]: (SPI)
static const USART_Config UC[USARTSNO] = {
    [0] = {.instance = USART3, .pclk_freq = 36000000, .UIRQn = USART3_IRQn, .DIRQn = DMA1_Channel2_IRQn, .dma_controller = DMA1, .dma_rx_channel = DMA1_Channel3, .dma_tx_channel = DMA1_Channel2, .TTCflag = DMA_ISR_TCIF2, .DEport = GPIOB, .DEpin = 1<<14 },
    [1] = {.instance = USART1, .pclk_freq = 72000000, .UIRQn = USART1_IRQn, .DIRQn = DMA1_Channel4_IRQn, .dma_controller = DMA1, .dma_rx_channel = DMA1_Channel5, .dma_tx_channel = DMA1_Channel4, .TTCflag = DMA_ISR_TCIF4, .DEport = GPIOB, .DEpin = 1<<0  },
    [2] = {.instance = USART2, .pclk_freq = 36000000, .UIRQn = USART2_IRQn, .DIRQn = DMA1_Channel7_IRQn, .dma_controller = DMA1, .dma_rx_channel = DMA1_Channel6, .dma_tx_channel = DMA1_Channel7, .TTCflag = DMA_ISR_TCIF7, .DEport = GPIOA, .DEpin = 1<<1  },
    [3] = {.instance = UART4,  .pclk_freq = 36000000, .UIRQn = UART4_IRQn,  .DIRQn = DMA2_Channel5_IRQn, .dma_controller = DMA2, .dma_rx_channel = DMA2_Channel3, .dma_tx_channel = DMA2_Channel5, .TTCflag = DMA_ISR_TCIF5 },
    [4] = {.instance = UART5,  .pclk_freq = 36000000, .UIRQn = UART5_IRQn }, // no DMA
};

// buffers for DMA or interrupt-driven data management
static uint8_t  inbuffers[USARTSNO][DMARXBUFSZ];
static uint16_t inbufidx[USARTSNO] = {0};       // for interrupt-driven - index of next character (also amount of received bytes)
static uint8_t  outbuffers[USARTSNO][DMATXBUFSZ];
static uint16_t outbufidx[USARTSNO] = {0};      // index of next char to transmit over interrupt
static uint16_t outbuflen[USARTSNO] = {0};      // length of data to transmit over interrupt [equal 0 if nothing to send]
static uint8_t  need2send[USARTSNO] = {0};      // flags from IDLE interrupt to send data portion
static uint8_t  TXrdy[USARTSNO] = {1,1,1,1,1};  // TX DMA ready
static int      dma_read_idx[USARTSNO] = {0};   // start of data in DMA inbuffers
// there's no way to tell recipient about overfull, so we will just "eat" spare data!


/**
 * @brief usart_config - configure US[A]RT based on usb_LineCoding
 * @param ifNo - interface index [0..4]
 * @param lc (io) - linecoding (modified to real speeds)
 */
void usart_config(uint8_t ifNo, usb_LineCoding *lc){
    // all clocking and GPIO config should be done in gpio_setup()!
    if(ifNo >= USARTSNO || UC[ifNo].instance == NULL) return;
    const USART_Config *cfg = &UC[ifNo];
    volatile USART_TypeDef *U = cfg->instance;
    uint32_t peripheral_clock = cfg->pclk_freq;
    // Disable USART while configuring
    U->CR1 = 0;
    U->ICR = 0xFFFFFFFF;   // Clear all interrupt flags
    // Assuming oversampling by 16 (default after reset). For higher baud rates you might use by 8.
    U->BRR = peripheral_clock / lc->dwDTERate;
    lc->dwDTERate = peripheral_clock / U->BRR;
    DBGs("New speed: "); DBGs(u2str(lc->dwDTERate)); DBGn();

    // ----- Data bits & Parity -----
    uint32_t cr1 = 0;
    uint32_t data_bits = lc->bDataBits;
    uint32_t parity_type = lc->bParityType;
    // Parity enable/disable and type
    if(parity_type != USB_CDC_NO_PARITY){
        cr1 |= USART_CR1_PCE;           // Parity enabled
        if(parity_type == USB_CDC_ODD_PARITY){
            cr1 |= USART_CR1_PS;        // Odd parity
        // MARK and SPACE parity are not natively supported; treat as even or ignore.
        }else if(parity_type == USB_CDC_MARK_PARITY || parity_type == USB_CDC_SPACE_PARITY){
            lc->bParityType = USB_CDC_EVEN_PARITY;
        }
    }
    DBGs("Parity: "); DBGch('0'+lc->bParityType); DBGn();

    // Word length (M bits) depends on data bits and parity
    // M[1:0] encoding: 00 = 8 bits, 01 = 9 bits, 10 = 7 bits
    if(cr1 & USART_CR1_PCE){
        // Parity enabled -> total frame length = data bits + 1 parity bit
        if(data_bits == 6){
            // 6 data + 1 parity = 7 bits total -> M=10 (7-bit mode)
            cr1 |= USART_CR1_M1;
        }else if(data_bits == 7){
            // 7 data + 1 parity = 8 bits total -> M=00 (8-bit mode)
            // do nothing
        }else{
            // 8 or 9 data bits with parity would be 9/10 bits total
            // Fallback to 8 data + parity
            cr1 |= USART_CR1_M0;
            lc->bDataBits = 8; // ??? need to be tested
        }
    }else{
        // Parity disabled
        if(data_bits == 7){
            cr1 |= USART_CR1_M1;        // M1=1, M0=0 -> 7-bit mode
        }else if(data_bits == 8){
            // do nothing M=00
        }else{
            // Unsupported (5,6 or bits) -> fallback to 8 bits
            lc->bDataBits = 8;
        }
    }
    DBGs("Data bits: "); DBGch('0' + lc->bDataBits); DBGn();

    // ----- Stop bits -----
    uint32_t cr2 = 0;
    switch(lc->bCharFormat){ // usb_LineCoding have no 0.5 stop bits field
    case USB_CDC_1_5_STOP_BITS:
        cr2 |= USART_CR2_STOP_0 | USART_CR2_STOP_1; // 1.5 stop bits -> CR2=11
        break;
    case USB_CDC_2_STOP_BITS:
        cr2 |= USART_CR2_STOP_1; // 2 stop bits -> CR2=10
        break;
    default:
        // do nothing: default to 1 stop bit -> CR2=00
        break;
    }
    DBGs("Stop bits: "); DBGch('0'+lc->bCharFormat); DBGn();

    // Write CR2 (stop bits)
    U->CR2 = cr2;
    // Enable transmitter, receiver, and interrupts (optional)
    cr1 |= USART_CR1_RE | USART_CR1_IDLEIE; // enable idle interrupt to force small portions of data into ringbuffer
    if(cfg->DEport){
        DBG("485 -> RX");
        RX485(cfg->DEport, cfg->DEpin);
        cr1 |= USART_CR1_TCIE;
    }else cr1 |= USART_CR1_TE;

    // ----- DMA -----
    if(cfg->dma_controller){ // DMA-driven
        DBG("DMA-driven");
        volatile DMA_Channel_TypeDef *T = cfg->dma_tx_channel, *R = cfg->dma_rx_channel;
        // Tx DMA
        T->CCR = 0;
        T->CPAR = (uint32_t) &U->TDR;
        T->CCR = DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE;
        // Rx DMA
        R->CCR = 0;
        R->CPAR = (uint32_t) &U->RDR;
        R->CMAR = (uint32_t) inbuffers[ifNo];
        R->CNDTR = DMARXBUFSZ;
        R->CCR = DMA_CCR_MINC | DMA_CCR_CIRC | DMA_CCR_EN; // manage circular buffer in polling
        // enable U[S]ART DMA
        U->CR3 = USART_CR3_DMAT | USART_CR3_DMAR;
    }else{
        DBG("IRQ-driven");
        cr1 |= USART_CR1_RXNEIE; // interrupt-driven
        inbufidx[ifNo] = 0;
        outbufidx[ifNo] = 0;
    }

    U->CR1 = cr1;
    // Wait for the idle frame to complete (optional)
    uint32_t tmout = 16000000;
    while(!(U->ISR & USART_ISR_TC)){
        if (--tmout == 0) break;
    }
    U->ICR = 0xFFFFFFFF;   // Clear flags again
    TXrdy[ifNo] = 1;
    DBGch('0' + ifNo);
    DBG("U[S]ART configured");
}

// start when received DTR
void usart_start(uint8_t ifNo){
    if(ifNo >= USARTSNO || UC[ifNo].instance == NULL) return;
    const USART_Config *cfg = &UC[ifNo];
    cfg->instance->CR1 |= USART_CR1_UE;
    NVIC_EnableIRQ(cfg->UIRQn);
    if(cfg->dma_controller){ // reset Rx DMA
        volatile DMA_Channel_TypeDef *R = cfg->dma_rx_channel;
        dma_read_idx[ifNo] = 0;
        R->CCR = 0;
        R->CPAR = (uint32_t) &cfg->instance->RDR;
        R->CMAR = (uint32_t) inbuffers[ifNo];
        R->CNDTR = DMARXBUFSZ;
        R->CCR = DMA_CCR_MINC | DMA_CCR_CIRC | DMA_CCR_EN;
        NVIC_EnableIRQ(cfg->DIRQn);
    }
    DBGch('0' + ifNo);
    DBG("U[S]ART started");
}

/**
 * @brief usart_stop - turn off U[S]ART for given interface
 * @param ifNo - interface number
 */
void usart_stop(uint8_t ifNo){
    if(ifNo >= USARTSNO || UC[ifNo].instance == NULL) return;
    const USART_Config *cfg = &UC[ifNo];
    cfg->instance->CR1 &= ~USART_CR1_UE;
    if(cfg->dma_controller){
        NVIC_DisableIRQ(cfg->DIRQn);
    }
    NVIC_DisableIRQ(cfg->UIRQn);
    if(cfg->DEport) RX485(cfg->DEport, cfg->DEpin);
    DBGch('0' + ifNo);
    DBG("U[S]ART stopped");
}

/**
 * @brief usarts_process - send/receive processing
 * Try to send data from output ringbuffer, check input DMA buffer and full input ringbuffer
 */
void usarts_process(){
    for(int i = 0; i < USARTSNO; ++i){ // index by interfaces number!!!
        const USART_Config *cfg = &UC[i];
        if(!(cfg->instance->CR1 & USART_CR1_UE)) continue; // USART disabled
        if(cfg->dma_controller){ // DMA-driven
            // Input data
            int write_idx = DMARXBUFSZ - cfg->dma_rx_channel->CNDTR; // next symbol to be written
            int available = (write_idx - dma_read_idx[i] + DMARXBUFSZ) % DMARXBUFSZ; // length of data available
            if(need2send[i] && available == 0) need2send[i] = 0;
            if(available >= USB_TXBUFSZ || need2send[i]){ // enough data or lonely couple of bytes
                // copy data in one or two chunks (wrap handling)
                if(dma_read_idx[i] + available <= DMARXBUFSZ){ // head before tail
                    USB_send(i, &inbuffers[i][dma_read_idx[i]], available);
                }else{ // head after tail - two chunks
                    uint32_t first = DMARXBUFSZ - dma_read_idx[i];
                    USB_send(i, &inbuffers[i][dma_read_idx[i]], first);
                    USB_send(i, inbuffers[i], available - first);
                }
                DBG("USART -> USB over DMA");
                dma_read_idx[i] = write_idx; // update read pointer
            }
#if 0
            if(DMARXBUFSZ - cfg->dma_rx_channel->CNDTR > DMARXBUFSZ/2 || need2send[i]){
               volatile DMA_Channel_TypeDef *R = cfg->dma_rx_channel;
                R->CCR &= ~DMA_CCR_EN; // pause DMA input transactions
                register int l = DMARXBUFSZ - R->CNDTR;
                if(l){ // have some input data -> send and restart DMA
                    DBG("need");
                    if(USB_send(i, inbuffers[i], l)){
                        DBG("USART -> USB over DMA");
                        // restart DMA only in case of succesfull sent or if failed, but have ability of buffer overfull
                        R->CMAR = (uint32_t) inbuffers[i];
                        R->CNDTR = DMARXBUFSZ;
                        need2send[i] = 0;
                    }
                }
                R->CCR |= DMA_CCR_EN; // re-enable DMA
            }
#endif
            // Output data
            if(TXrdy[i]){ // ready to send new data
                int got = USB_receive(i, outbuffers[i], DMATXBUFSZ);
                if(got > 0){ // send next data portion
                    volatile DMA_Channel_TypeDef *T = cfg->dma_tx_channel;
                    //cfg->dma_controller->IFCR = cfg->TTCflag; // now we can clear TC flag (TC and CTC are the same)
                    T->CCR &= ~DMA_CCR_EN;
                    T->CMAR = (uint32_t) outbuffers[i];
                    T->CNDTR = got;
                    if(cfg->DEport){ // switch to Tx
                        DBG("485 -> TX");
                        TX485(cfg->DEport, cfg->DEpin);
                        cfg->instance->CR1 &= ~USART_CR1_RE;
                        cfg->instance->CR1 |= USART_CR1_TE;
                    }
                    TXrdy[i] = 0;
                    T->CCR |= DMA_CCR_EN; // start new transmission
                    DBG("USB -> USART over DMA");
                }
            }
        }else{ // interrupt-driven
            // Input data
            volatile USART_TypeDef *U = cfg->instance;
            U->CR1 &= ~USART_CR1_RXNEIE; // temporarily disable interrupt
            register int l = inbufidx[i];
            if(l > DMARXBUFSZ/2 || need2send[i]){
                if(l && USB_send(i, inbuffers[i], l)){
                    need2send[i] = 0;
                    inbufidx[i] = 0;
                    DBG("USART -> USB over irq");
                }
            }
            U->CR1 |= USART_CR1_RXNEIE; // restore irq reaction
            // output data
            if(TXrdy[i]){
                int got = USB_receive(i, outbuffers[i], DMATXBUFSZ);
                if(got > 0){
                    if(cfg->DEport){ // switch to Tx
                        DBG("485 -> TX");
                        TX485(cfg->DEport, cfg->DEpin);
                        U->CR1 &= ~USART_CR1_RE;
                        U->CR1 |= USART_CR1_TE;
                    }
                    outbufidx[i] = 1; // continue from next symbol
                    outbuflen[i] = got;
                    TXrdy[i] = 0;
                    U->TDR = outbuffers[i][0]; // start transmission
                    U->CR1 |= USART_CR1_TXEIE; // enable TXE interrupt
                    DBG("USB -> USART over irq");
                }
            }
        }
    }
}

// Use this function only for debug purpose
int usart_send(uint8_t ifNo, const uint8_t *data, int len){
    if(ifNo >= USARTSNO || !data || len < 1) return 0;
    const USART_Config *cfg = &UC[ifNo];
    if(TXrdy[ifNo] == 0 || !(cfg->instance->CR1 & USART_CR1_UE)) return -1; // busy or not active
    if(len > DMATXBUFSZ) len = DMATXBUFSZ;
    memcpy(outbuffers[ifNo], data, len);
    if(cfg->dma_controller){
        volatile DMA_Channel_TypeDef *T = cfg->dma_tx_channel;
        //cfg->dma_controller->IFCR = cfg->TTCflag; // now we can clear TC flag (TC and CTC are the same)
        T->CCR &= ~DMA_CCR_EN;
        T->CMAR = (uint32_t) outbuffers[ifNo];
        T->CNDTR = len;
        if(cfg->DEport){ // switch to Tx
            DBG("485 -> TX");
            TX485(cfg->DEport, cfg->DEpin);
            cfg->instance->CR1 &= ~USART_CR1_RE;
            cfg->instance->CR1 |= USART_CR1_TE;
        }
        TXrdy[ifNo] = 0;
        T->CCR |= DMA_CCR_EN; // start new transmission
    }else{
        volatile USART_TypeDef *U = cfg->instance;
        if(cfg->DEport){ // switch to Tx
            DBG("485 -> TX");
            TX485(cfg->DEport, cfg->DEpin);
            U->CR1 &= ~USART_CR1_RE;
            U->CR1 |= USART_CR1_TE;
        }
        outbufidx[ifNo] = 1; // continue from next symbol
        outbuflen[ifNo] = len;
        U->TDR = outbuffers[ifNo][0]; // start transmission
        U->CR1 |= USART_CR1_TXEIE; // enable TXE interrupt
    }
    return len;
}

/**
 * @brief usart_isr - U[S]ART interrupt: IDLE (for DMA-driven) or
 * @param ifno - interface index
 */
static void usart_isr(uint8_t ifno){
    const USART_Config *cfg = &UC[ifno];
    volatile USART_TypeDef *U = cfg->instance;
    // for every flag we should also check if it's IRQ active
    if((U->ISR & USART_ISR_RXNE) && (U->CR1 & USART_CR1_RXNEIE)){ // got new byte
        if(inbufidx[ifno] == DMARXBUFSZ) (void) U->RDR; // throw away data: buffer overfull
        else inbuffers[ifno][ inbufidx[ifno]++ ] = U->RDR; // put new byte into buffer
    }
    // IDLE active for both DMA- and interrupt-driven transitions
    if(U->ISR & USART_ISR_IDLE){ // try to send collected data (DMA-driven)
        need2send[ifno] = 1; // seems like data portion is over - try to send it
        U->ICR = USART_ICR_IDLECF;
    }
    if((U->ISR & USART_ISR_TXE) && (U->CR1 & USART_CR1_TXEIE)){ // send next byte if need (interrupt-driven)
        if(outbuflen[ifno] > outbufidx[ifno]){
            U->TDR = outbuffers[ifno][ outbufidx[ifno]++ ];
        }else if(U->CR1 & USART_CR1_TXEIE){
            U->CR1 &= ~USART_CR1_TXEIE; // disable interrupt: no data to send
            TXrdy[ifno] = 1;
        }
    }
    if(U->ISR & USART_ISR_TC){ // switch RS-485 to Rx after transmission complete
        if(cfg->DEport){
            RX485(cfg->DEport, cfg->DEpin);
            U->CR1 &= ~USART_CR1_TE;
            U->CR1 |= USART_CR1_RE;
        }
        U->ICR = USART_ICR_TCCF;
    }
}

// U[S]ART interrupts (for DMA-driven - only IDLE, for interrupt-driven: RXNE and TXE)
void usart1_exti25_isr(){ usart_isr(1); }
void usart2_exti26_isr(){ usart_isr(2); }
void usart3_exti28_isr(){ usart_isr(0); }
void uart4_exti34_isr(){  usart_isr(3); }
void uart5_exti35_isr(){  usart_isr(4); }

// DMA Tx interrupts (to arm ready flag)
void dma1_channel2_isr(){ TXrdy[0] = 1; DMA1->IFCR = DMA_IFCR_CTCIF2; }
void dma1_channel4_isr(){ TXrdy[1] = 1; DMA1->IFCR = DMA_IFCR_CTCIF4; }
void dma1_channel7_isr(){ TXrdy[2] = 1; DMA1->IFCR = DMA_IFCR_CTCIF7; }
void dma2_channel5_isr(){ TXrdy[3] = 1; DMA2->IFCR = DMA_IFCR_CTCIF5; }
