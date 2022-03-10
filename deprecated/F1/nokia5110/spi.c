/*
 * spi.c
 *
 * Copyright 2014 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include "main.h"
#include "spi.h"
#include "hw_init.h"

uint32_t Current_SPI = SPI1; // this is SPI interface which would b
/**
 * Set current SPI to given value
 */
void switch_SPI(uint32_t SPI){
	Current_SPI = SPI;
}

/*
 * Configure SPI ports
 */
/*
 *      SPI1 remapped:
 * SCK  - PB3
 * MISO - PB4
 * MOSI - PB5
 */
void SPI1_init(){
	// enable AFIO & other clocking
	rcc_peripheral_enable_clock(&RCC_APB2ENR,
			RCC_APB2ENR_SPI1EN | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPBEN);
	// remap SPI1 (change pins from PA5..7 to PB3..5); also turn off JTAG
	gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF, AFIO_MAPR_SPI1_REMAP);
	// SCK, MOSI - push-pull output
	gpio_set_mode(GPIO_BANK_SPI1_RE_SCK, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_SPI1_RE_SCK);
	gpio_set_mode(GPIO_BANK_SPI1_RE_MOSI, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_SPI1_RE_MOSI);
	// MISO - opendrain in
	gpio_set_mode(GPIO_BANK_SPI1_RE_MISO, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_SPI1_RE_MISO);
	spi_reset(SPI1);
	/* Set up SPI in Master mode with:
	 * Clock baud rate: 1/128 of peripheral clock frequency (APB2, 72MHz)
	 * Clock polarity: CPOL=0, CPHA=0
	 * Data frame format: 8-bit
	 * Frame format: MSB First
	 */
	spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_256, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
		SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	nvic_enable_irq(NVIC_SPI1_IRQ); // enable SPI interrupt
	spi_enable(Current_SPI);
}

/*
 *     SPI2:
 * SCK  - PB13
 * MISO - PB14
 * MOSI - PB15
 */
void SPI2_init(){
	// turn on clocking
	//rcc_periph_clock_enable(RCC_SPI2 | RCC_GPIOB);
	rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_SPI2EN);
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPBEN);
	// SCK, MOSI - push-pull output
	gpio_set_mode(GPIO_BANK_SPI2_SCK, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_SPI2_SCK);
	gpio_set_mode(GPIO_BANK_SPI2_MOSI, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_SPI2_MOSI);
	// MISO - opendrain in
	gpio_set_mode(GPIO_BANK_SPI2_MISO, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_SPI2_MISO);
	spi_reset(SPI2);
	/* Set up SPI in Master mode with:
	 * Clock baud rate: 1/64 of peripheral clock frequency (APB1, 36MHz)
	 * Clock polarity: Idle High
	 * Clock phase: Data valid on 2nd clock pulse
	 * Data frame format: 8-bit
	 * Frame format: MSB First
	 */
	spi_init_master(SPI2, SPI_CR1_BAUDRATE_FPCLK_DIV_64, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
		SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
//	nvic_enable_irq(NVIC_SPI2_IRQ); // enable SPI interrupt
	spi_enable(Current_SPI);
}

void SPI_init(){
	switch(Current_SPI){
		case SPI1:
			SPI1_init();
		break;
		case SPI2:
			SPI2_init();
		break;
		default:
		return; // error
	}
}

/**
 * send 1 byte blocking
 * return 1 on success
 */
uint8_t spi_write_byte(uint8_t data){
	while(!(SPI_SR(Current_SPI) & SPI_SR_TXE));
	SPI_DR(Current_SPI) = data;
	while(!(SPI_SR(Current_SPI) & SPI_SR_TXE));
	while(!(SPI_SR(Current_SPI) & SPI_SR_RXNE));
	(void)SPI_DR(Current_SPI);
	while(!(SPI_SR(Current_SPI) & SPI_SR_TXE));
	while(SPI_SR(Current_SPI) & SPI_SR_BSY);
	return 1;
}

/**
 * Blocking rite data to current SPI
 * @param data - buffer with data
 * @param len  - buffer length
 * @return 0 in case of error (or 1 in case of success)
 */
uint8_t spiWrite(uint8_t *data, uint16_t len){
	uint16_t i;
	while(!(SPI_SR(Current_SPI) & SPI_SR_TXE));
	for(i = 0; i < len; ++i){
		SPI_DR(Current_SPI) = data[i];
		while(!(SPI_SR(Current_SPI) & SPI_SR_TXE));
	}
	while(!(SPI_SR(Current_SPI) & SPI_SR_RXNE));
	(void)SPI_DR(Current_SPI);
	while(!(SPI_SR(Current_SPI) & SPI_SR_TXE));
	while(SPI_SR(Current_SPI) & SPI_SR_BSY);
	return 1;
}

/*
// SPI interrupt
void spi_isr(uint32_t spi){
	// TX empty
	if(SPI_SR(spi) & SPI_SR_TXE){
		if(SPI_TxIndex < SPI_datalen){ // buffer still not sent fully
		// Send Transaction data
			SPI_DR(spi) = SPI_TxBuffer[SPI_TxIndex++];
		}else{ // disable TXE interrupt + set EOT flag
			spi_disable_tx_buffer_empty_interrupt(spi);
			//spi_disable(spi);
			SPI_EOT_FLAG = 1;
			while(SPI_SR(spi) & SPI_SR_BSY);
		}
	}
	SPI_SR(spi) = 0; // clear all interrupt flags
}

// interrupts for SPI1 & SPI2
void spi1_isr(){
	spi_isr(SPI1);
}

void spi2_isr(){
	spi_isr(SPI2);
}
*/
