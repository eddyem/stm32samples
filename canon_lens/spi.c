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

/*
 * Configure SPI ports
 */
/*
 *      SPI1 remapped:
 * SCK  - PB3
 * MISO - PB4
 * MOSI - PB5
 *
 */
void SPI_init(){
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
	rcc_set_ppre2(RCC_CFGR_PPRE2_HCLK_DIV4);
	spi_reset(SPI1);
	/* Set up SPI in Master mode with:
	 * for Canon lens SPI settings are: f~78kHz, CPOL=1, CPHA=1
	 * Clock baud rate: 1/256 of peripheral clock frequency (APB2, 72MHz/4 = 18MHz) - ~70kHz
	 * Clock polarity: CPOL=1, CPHA=1
	 * Data frame format: 8-bit
	 * Frame format: MSB First
	 */
	spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_256, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,
		SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	//nvic_enable_irq(NVIC_SPI1_IRQ); // enable SPI interrupt
	spi_enable(SPI1);
}

/**
 * send 1 byte blocking
 * return readed byte on success
 */
uint8_t spi_write_byte(uint8_t data){
	while(!(SPI_SR(SPI1) & SPI_SR_TXE));
	SPI_DR(SPI1) = data;
	while(!(SPI_SR(SPI1) & SPI_SR_TXE));
	while(!(SPI_SR(SPI1) & SPI_SR_RXNE));
	uint8_t rd = SPI_DR(SPI1);
	while(!(SPI_SR(SPI1) & SPI_SR_TXE));
	while(SPI_SR(SPI1) & SPI_SR_BSY);
	return rd;
}


