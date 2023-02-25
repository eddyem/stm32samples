/*
 * This file is part of the multistepper project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "hardware.h"
#include "proto.h"

extern volatile uint32_t Tms;

#define MAXBUFLEN           (8)
// timeout, milliseconds
#define PDNU_TMOUT          (5)

// buffers format: 0 - sync+reserved, 1 - address (0..3 - slave, 0xff - master)
//      2 - register<<1 | RW, 3 - CRC (r) or [ 3..6 - MSB data, 7 - CRC ]
// buf[0] - USART2, buf[1] - USART3
//static uint8_t notfound = 0; // not found mask (LSB - 0, MSB - 7)

// datalen == 3 for read request or 7 for writing
static void calcCRC(uint8_t *outbuf, int datalen){
    uint8_t crc = 0;
    for(int i = 0; i < datalen; ++i){
        uint8_t currentByte = outbuf[i];
        for(int j = 0; j < 8; ++j){
            if((crc >> 7) ^ (currentByte & 0x01)) crc = (crc << 1) ^ 0x07;
            else crc <<= 1;
            currentByte = currentByte >> 1;
        }
    }
    outbuf[datalen] = crc;
}

static volatile USART_TypeDef *USART[2] = {USART2, USART3};

static void setup_usart(int no){
    USART[no]->ICR = 0xffffffff; // clear all flags
    USART[no]->BRR = 72000000 / 256000; // 256 kbaud
    USART[no]->CR3 = USART_CR3_HDSEL; // enable DMA Tx/Rx, single wire
    USART[no]->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; // 1start,8data,nstop; enable Rx,Tx,USART
    uint32_t tmout = 16000000;
    while(!(USART[no]->ISR & USART_ISR_TC)){if(--tmout == 0) break;} // polling idle frame Transmission
    USART[no]->ICR = 0xffffffff; // clear all flags again
}

// USART2 (ch0..3), USART3 (ch4..7)
// pins are setting up in `hardware.c`
void pdnuart_setup(){
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_USART3EN;
    setup_usart(0);
    setup_usart(1);
}

static int rwreg(uint8_t motorno, uint8_t reg, uint32_t data, int w){
    if(motorno >= MOTORSNO || reg & 0x80) return FALSE;
    int no = motorno >> 2;
    uint8_t outbuf[MAXBUFLEN];
    outbuf[0] = 0xa0;
    outbuf[1] = motorno - (no << 2);
    outbuf[2] = reg << 1;
    int nbytes = 3;
    if(w){
        outbuf[2] |= 1;
        for(int i = 6; i > 2; --i){
            outbuf[i] = data & 0xff;
            data >>= 8;
        }
        nbytes = 7;
    }
    calcCRC(outbuf, nbytes);
    ++nbytes;
    for(int i = 0; i < nbytes; ++i){
        USART[no]->TDR = outbuf[i]; // transmit
        while(!(USART[no]->ISR & USART_ISR_TXE));
    }
    return TRUE;
}

// return FALSE if failed
int pdnuart_writereg(uint8_t motorno, uint8_t reg, uint32_t data){
    return rwreg(motorno, reg, data, 1);
}

// return FALSE if failed
int pdnuart_readreg(uint8_t motorno, uint8_t reg, uint32_t *data){
    if(!rwreg(motorno, reg, 0, 0)) return FALSE;
    uint32_t Tstart = Tms;
    uint8_t buf[8];
    int no = motorno >> 2;
    for(int i = 0; i < 8; ++i){
        while(!(USART[no]->ISR & USART_ISR_RXNE))
            if(Tms - Tstart > PDNU_TMOUT) return FALSE;
        buf[i] = USART[no]->RDR;
        USB_sendstr("byte: "); printuhex(buf[i]); newline();
    }
    uint32_t o = 0;
    for(int i = 3; i < 7; ++i){
        o |= buf[i];
        o <<= 8;
    }
    *data = o;
    return TRUE;
}

/*
static void parseRx(int no){
    USB_sendstr("Got from ");
    USB_putbyte('#'); printu(curslaveaddr[no] + no*4); USB_sendstr(": ");
    for(int i = 0; i < 8; ++i){
        printuhex(inbuf[no][i]); USB_putbyte(' ');
    }
    newline();
}
*/
