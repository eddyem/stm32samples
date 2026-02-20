/*
 * This file is part of the multiiface project.
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

#include "Debug.h"
#include "hardware.h"
#include "spi.h"

typedef enum{
    SPI_NOTREADY,
    SPI_IDLE,
    SPI_READY,
#ifdef SPIDMA
    SPI_BUSY
#endif
} spiStatus;

static spiStatus spi_status = SPI_NOTREADY;
static uint32_t spidata = 0;

#define WAITX(x)  do{volatile uint32_t  wctr = 0; while((x) && (++wctr < 360000)) IWDG->KR = IWDG_REFRESH; if(wctr==360000){ DBG("timeout"); return 0;}}while(0)

// init SPI to work with and without DMA
// DMA1Channel2 - SPI1 Rx
void spi_setup(){
    SPI1->CR1 = 0; // clear EN
    SPI1->CR2 = 0;
    // SPI for SSI: PA5/PA6, without MOSI; suppose that clocking and GPIO OK (hardware.c)
    RCC->APB2RSTR = RCC_APB2RSTR_SPI1RST; // reset SPI before start
    RCC->APB2RSTR = 0; // clear reset
    SPI1->CR1 = SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_RXONLY; // software slave management (without hardware NSS pin); RX only
#ifdef SPIDMA
    // setup SPI DMA
    SPI11->CR2 = SPI_CR2_RXDMAEN;
    // Rx
    DMA1_Channel2->CPAR = (uint32_t)&(SPI1->DR);
    DMA1_Channel2->CCR = DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_TEIE; // mem inc, hw->mem, Rx complete and error interrupt
    NVIC_EnableIRQ(DMA1_Channel2_IRQn); // enable Rx interrupt
#endif
    // Master, baudrate = 0b110 - fpclk/128 (562.5 kHz)
    SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_BR_2 | SPI_CR1_BR_1;
    // DS=8bit; RXNE generates after 8bit of data in FIFO;
    SPI1->CR2 |= SPI_CR2_FRXTH | SPI_CR2_DS_2|SPI_CR2_DS_1|SPI_CR2_DS_0;
    spi_status = SPI_IDLE;
    DBG("SPI setup OK");
}

// return TRUE if data ready and change `encval`
int spi_read_enc(uint32_t *encval){
    if(spi_status != SPI_READY) return FALSE;
    spi_status = SPI_IDLE;
#ifndef SPIDMA
    // clear SPI Rx FIFO
    for(int i = 0; i < 4; ++i) (void) SPI1->DR;
    SPI1->CR1 |= SPI_CR1_SPE;
    uint8_t *data = (uint8_t*) &spidata;
    for(uint32_t x = 0; x < ENCODERBYTES; ++x){
        if(x == ENCODERBYTES - 1) SPI1->CR1 &= ~SPI_CR1_RXONLY; // clear RXonly bit to stop CLK generation after next byte
        WAITX(!(SPI1->SR & SPI_SR_RXNE));
        data[x] = *((volatile uint8_t*)&SPI1->DR);
    }
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 |= SPI_CR1_RXONLY; // and return RXonly bit
#endif
    if(encval) *encval = spidata;
    return TRUE;
}

#ifdef SPIDMA
// start encoder reading over DMA
// @return FALSE if SPI is busy
int spi_start_enc(){
    if(spi_status == SPI_BUSY || spi_status == SPI_NOTREADY) return FALSE;
    if(SPI1->SR & SPI_SR_BSY) return FALSE;
    DMA1_Channel2->CMAR = (uint32_t) &spidata;
    DMA1_Channel2->CNDTR = 4;
    DMA1_Channel2->CCR |= DMA_CCR_EN;
    SPI1->CR1 |= SPI_CR1_SPE;
    spi_status = SPI_BUSY;
    return TRUE;
}

// SSI got fresh data
void dma1_channel2_isr(){
    SPI1->CR1 &= ~SPI_CR1_SPE;
    spi_status = SPI_READY; // ready independent on errors or Rx ready
}
#else
int spi_start_enc(){ // simple stub
    if(spi_status == SPI_NOTREADY) return FALSE;
    spi_status = SPI_READY; // user asks to read data
    return TRUE;
}
#endif
