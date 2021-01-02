/*
 * This file is part of the ws2815 project.
 * Copyright 2020 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "ws2815.h"
#include "hardware.h"
#include "usb.h"

// DMA buffer size (24*2*N)
// if DMAHALFBUFSIZE differs from 24, convertcolor should be changed!!!
#define DMABUFSIZE  (48)
#define DMAHALFBUFSIZE (24)

// buffer for DMA transfers (two LEDs)
static uint8_t dmabuf[DMABUFSIZE];

// buffer for GRB colors
static uint32_t colorbuf[LEDS_NUM];
static int currLED = 0; // currrent led number

// change color of led with number LEDno
/**
 * @brief setcolr - change color of led with number LEDno
 * @param LEDno - led number in array
 * @param colr  - XXRRGGBB color
 * @return 1 if all OK
 */
int ws2815setpix(uint16_t LEDno, uint32_t colr){
    if(LEDno > LEDS_NUM - 1) return 0;
    if(colr & 0xff000000) colr &= 0xffffff;
    colorbuf[LEDno] = colr;
    return 1;
}

uint32_t ws2815getpix(uint16_t LEDno){
    if(LEDno > LEDS_NUM - 1) return 0;
    return colorbuf[LEDno];
}

/**
 * @brief convertcolor - convert color from XXRRGGBB to dmabuf
 * @param halfno - number of buffer half (0/1)
 * @param colr - 24bit color
 */
static void convertcolor(int halfno, uint32_t colr){
    // check LED number
    if(++currLED > LEDS_NUM){
        TIM1->CR1 |= TIM_CR1_OPM;
        // THIS IS A DIRTY HACK! IT WORKS ONLY @ HIGH SPEEDS, WHEN THERE'S NO TIME TO GO INTO IRQ
        // On small frequencies comment this line and allow CC1 IRQ
        TIM1->CCR1 = 0;
        currLED = 0;
        return;
    }
#if 0
    // XXRRGGBB - 8bit
    uint8_t GRB[3], X;
    X = (colr >> 5)&6; // number to bits for shift (XX*2)
    GRB[1] = (colr>>4) << X; // R
    GRB[0] = ((colr>>2) & 3) << X; // G
    GRB[2] = (colr & 3) << X; // B
#endif
    uint8_t *dptr = &dmabuf[DMAHALFBUFSIZE*halfno], *cptr = (uint8_t*)&colr; //*cptr = GRB;
    for(int i = 0; i < DMAHALFBUFSIZE; i+=8){
        uint8_t octet = *cptr++;
        for(int j = 7; j > -1; --j){
            *dptr++ = (octet & (1<<j)) ? LONGPULSE : SHORTPULSE;
        }
    }
}

void ws2815start(){
    WS2815TIM->CR1 = 0; // stop timer
    WS2815DMAch->CCR &= ~DMA_CCR_EN; // disable DMA to reconfigure
    convertcolor(0, colorbuf[currLED]);
    if(currLED < LEDS_NUM - 1)
        convertcolor(1, colorbuf[currLED]);
    WS2815DMA->IFCR = WS2815DMA_IFCR_CLR; // clear interrupt flags
    WS2815DMAch->CNDTR = DMABUFSIZE;
    WS2815DMAch->CMAR = (uint32_t)dmabuf;
    WS2815DMAch->CCR |= DMA_CCR_EN; // start DMA
    WS2815TIM->CR1 = TIM_CR1_CEN | TIM_CR1_URS;
}

/*
// timer update event (end of reset pulse -> start data sending process)
void WSTMCCISR(){
    TIM1->SR = 0;
    TIM1->CCR1 = 0;
    TIM1->DIER = 0;
}*/

// DMA half/full transfer interrupts: fill another half of buffer
void WSDMAISR(){
    if(WS2815DMA->ISR & WS2815DMA_ISR_HTIF){ // half transfer - fill first half
        convertcolor(0, colorbuf[currLED]);
    }else if(WS2815DMA->ISR & WS2815DMA_ISR_TCIF){ // transfer complete - fill second half
        convertcolor(1, colorbuf[currLED]);
    }
    WS2815DMA->IFCR = WS2815DMA_IFCR_CLR;
}
