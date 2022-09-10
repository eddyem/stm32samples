/*
 * This file is part of the canonmanage project.
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

#pragma once
#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include <stm32f1.h>

// LED0 - PC13 (bluepill), blinking each second
#define LED0_port       GPIOC
#define LED0_pin        (1<<13)

#define LED_blink(x)    pin_toggle(x ## _port, x ## _pin)
#define LED_on(x)       pin_clear(x ## _port, x ## _pin)
#define LED_off(x)      pin_set(x ## _port, x ## _pin)

#define USBPU_port  GPIOA
#define USBPU_pin   (1<<15)
#define USBPU_ON()  pin_set(USBPU_port, USBPU_pin)
#define USBPU_OFF() pin_clear(USBPU_port, USBPU_pin)

extern volatile uint32_t Tms;

void hw_setup();

// SPI RX/TX max len
#define SPIBUFSZ        (32)

#define DMA_SPI_Rx_IRQ  DMA1_Channel2_IRQn
#define SPIx            SPI1
#define SPI_APB2        RCC_APB2ENR_SPI1EN
#define DMA_SPI         DMA1
#define DMA_SPI_AHBENR  RCC_AHBENR_DMA1EN
#define DMA_SPI_TCIF    DMA_ISR_TCIF2
#define DMA_SPI_CTCIF   DMA_IFCR_CTCIF2
#define DMA_SPI_TxChannel DMA1_Channel3
#define DMA_SPI_RxChannel DMA1_Channel2
#define DMA_SPI_Rx_ISR  dma1_channel2_isr

#endif // __HARDWARE_H__
