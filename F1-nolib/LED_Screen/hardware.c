/*
 *                                                                                                  geany_encoding=koi8-r
 * hardware.c - hardware-dependent macros & functions
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 *
 */

#include "hardware.h"
#include "spi.h"

static inline void gpio_setup(){
    // Enable clocks to the GPIO subsystems, turn on AFIO clocking to disable SWD/JTAG
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
    // turn off USB pullup
    GPIOA->ODR = (1<<15);
    // Set led as opendrain output
    GPIOC->CRH = CRH(13, CNF_ODOUTPUT|MODE_SLOW);
    // USB pullup (PA15) - opendrain output
    // SCREEN PINs: A,B - PB6,PB7; SCLK - PA6; nOE - PA13
    GPIOA->CRH = CRH(15, CNF_PPOUTPUT|MODE_SLOW);// | CRH(13, CNF_PPOUTPUT|MODE_SLOW);
    // turn off SWJ/JTAG (PA13 is in use)
   // AFIO->MAPR = AFIO_MAPR_SWJ_CFG_DISABLE;
    GPIOB->CRL = CRL(6, CNF_PPOUTPUT|MODE_SLOW) | CRL(7, CNF_PPOUTPUT|MODE_SLOW);
    GPIOA->CRL = CRL(6, CNF_PPOUTPUT|MODE_SLOW) | CRL(4, CNF_PPOUTPUT|MODE_SLOW);
}

void hw_setup(){
    gpio_setup();
    spi_setup();
}

// SPI1 DMA Tx interrupt
void dma1_channel3_isr(){
  if(DMA1->ISR & DMA_ISR_TCIF3){
    DMA1->IFCR |= DMA_IFCR_CTCIF3; /* Clear TC flag */
    SPI_status = SPI_READY;
    DMA_SPI_Channel->CCR &=~ DMA_CCR_EN; // turn off DMA for further reconfiguration
  }
}
