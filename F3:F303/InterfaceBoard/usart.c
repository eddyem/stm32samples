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

#include "debug.h"
#include "hardware.h"
#include "strfunc.h"
#include "usart.h"
#include "usb_descr.h" // InterfacesAmount, IFx

static volatile int idatalen = 0; // received data line length (including '\n')
// USARTs registers by interface number [IF1..IF7], index=epNo-1
static volatile USART_TypeDef *USARTx[InterfacesAmount] = {USART3, USART1, USART2, UART4, UART5, NULL, NULL};
static int usartByIfNo[] = {0, ISerial1, ISerial2, ISerial0, ISerial3, ISerial4}; // USARTx -> index x
// APB1/APB2 bus speeds:
static const uint32_t usart_clocks[INTERFACES_AMOUNT] = {
    72000000,  // USART3 on APB2 (72 MHz)
    36000000,  // USART1 on APB1 (36 MHz)
    72000000,  // USART2 on APB2 (72 MHz)
    72000000,  // UART4  on APB2 (72 MHz)
    72000000,  // UART5  on APB2 (72 MHz)
    0,         // not used
    0          // not used
};

#if 0
volatile int linerdy = 0,   // received data ready
    dlen    = 0,            // length of data (including '\n') in current buffer
    bufovr  = 0;            // input buffer overfull


static void usart_putchar(int no, uint8_t ch){
    while(!(USARTx[no]->ISR & USART_ISR_TXE));
    USARTx[no]->TDR = ch;
}
void usart_sendn(uint8_t ifNo, const uint8_t *str, int L){
    if (!str || L < 0 || ifNo < 1 || ifNo > USARTSNO)
        return;
    for(int i = 0; i < L; ++i){
        usart_putchar(ifNo, str[i]);
    }
}

// setup all USARTs
void usarts_setup(){
    // clock
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_USART3EN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    for(int i = 0; i < USARTSNO; ++i)
        usart_config(i + USART1_IDX, lineCodings);
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_EnableIRQ(USART3_IRQn);
}
#endif

// TODO: fixme for different settings
//lineCoding.dwDTERate = speeds[usartNo];
//lineCoding.bCharFormat = USB_CDC_1_STOP_BITS;
//lineCoding.bParityType = USB_CDC_NO_PARITY;
//lineCoding.bDataBits = 8;

/**
 * @brief usart_config - configure US[A]RT based on usb_LineCoding
 * @param ifNo - interface index [0..6]
 * @param lc (io) - linecoding (modified to real speeds)
 */
void usart_config(uint8_t ifNo, usb_LineCoding *lc){
    if(ifNo >= INTERFACES_AMOUNT || USARTx[ifNo] == NULL) return;
    volatile USART_TypeDef *U = USARTx[ifNo];
    uint32_t peripheral_clock = usart_clocks[ifNo];
    // Disable USART while configuring
    U->CR1 = 0;
    U->ICR = 0xFFFFFFFF;   // Clear all interrupt flags
    // Assuming oversampling by 16 (default after reset). For higher baud rates you might use by 8.
    U->BRR = peripheral_clock / lc->dwDTERate;
    lc->dwDTERate = peripheral_clock / U->BRR;

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
        }else if(data_bits == 8){
            // 8 data + 1 parity = 9 bits total -> M=01 (9-bit mode)
            cr1 |= USART_CR1_M0;        // M0=1, M1=0
        }else{
            // Unsupported (9 data bits with parity would be 10 bits total)
            // Fallback to 8 data + parity
            cr1 |= USART_CR1_M0;
            lc->bDataBits = 8;
        }
    }else{
        // Parity disabled
        if(data_bits == 7){
            cr1 |= USART_CR1_M1;        // M1=1, M0=0 -> 7-bit mode
        }else if(data_bits == 8){
            // do nothing M=00
        }else if(data_bits == 9){
            cr1 |= USART_CR1_M0;        // M0=1, M1=0 -> 9-bit mode
        }else{
            // Unsupported (5,6 bits) -> fallback to 8 bits
            lc->bDataBits = 8;
        }
    }

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

    // Write CR2 (stop bits)
    U->CR2 = cr2;
    // Enable transmitter, receiver, and RX interrupt (optional)
    cr1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_RXNEIE;
    U->CR1 = cr1;

    // Wait for the idle frame to complete (optional)
    uint32_t tmout = 16000000;
    while(!(U->ISR & USART_ISR_TC)){
        if (--tmout == 0) break;
    }
    U->ICR = 0xFFFFFFFF;   // Clear flags again
}

// TODO: leave only uart5_exti35_isr, other - over DMA
// UART5 [IF5] Tx - over finite-state machine

/*
DMA1 channels:
- Ch2: USART3_Tx [IF1]
- Ch3: USART3_Rx [IF1]
- Ch4: USART1_Tx [IF2]
- Ch5: USART1_Rx [IF2]
- Ch6: USART2_Rx [IF3]
- Ch7: USART2_Tx [IF3]

DMA2 channels:
- Ch3: UART4_Rx [IF4]
- Ch5: UART4_Tx [IF4]
*/

static void usart_isr(int usartno, volatile USART_TypeDef *U){
    int iface = usartByIfNo[usartno]; // interface
    if(U->ISR & USART_ISR_RXNE){
        USB_putbyte(iface, U->RDR);
    }
}

void usart1_exti25_isr(){
    usart_isr(1, USART1);
}

void usart2_exti26_isr(){
    usart_isr(2, USART2);
}

void usart3_exti28_isr(){
    usart_isr(3, USART3);
}

void uart4_exti34_isr(){
    usart_isr(4, UART4);
}

void uart5_exti35_isr(){
    usart_isr(5, UART5);
}
