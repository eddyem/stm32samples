/*
 * stm32f0.h
 *
 * Copyright 2017 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>

#define RCC_CFGR_PLLXTPRE_HSE_CLK		    0x0
#define RCC_CFGR_PLLXTPRE_HSE_CLK_DIV2		0x1
#define RCC_CFGR_PLLSRC_HSI_CLK_DIV2		0x0
#define RCC_CFGR_PLLSRC_HSE_CLK	    		0x1


inline void rcc_clock_setup_in_hse_8mhz_out_48mhz(void){
    RCC_CR |= RCC_CR_HSEON;
    while ((RCC_CIR & RCC_CIR_HSERDYF) != 0);
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSE;

    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_HPRE) | RCC_CFGR_HPRE_NODIV;
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_PPRE) | RCC_CFGR_PPRE_NODIV;

    FLASH_ACR = (FLASH_ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_024_048MHZ;

    /* PLL: 8MHz * 6 = 48MHz */
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_PLLMUL) | RCC_CFGR_PLLMUL_MUL6;
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_PLLSRC) | (RCC_CFGR_PLLSRC_HSE_CLK << 16);
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_PLLXTPRE) | (RCC_CFGR_PLLXTPRE_HSE_CLK << 17);

    RCC_CR |= RCC_CR_PLLON;
    while ((RCC_CIR & RCC_CIR_PLLRDYF) != 0);
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
}

inline void pin_toggle(uint32_t gpioport, uint16_t gpios){
    uint32_t port = GPIO_ODR(gpioport);
    GPIO_BSRR(gpioport) = ((port & gpios) << 16) | (~port & gpios);
}

inline void pin_set(uint32_t gpioport, uint16_t gpios){GPIO_BSRR(gpioport) = gpios;}
inline void pin_clear(uint32_t gpioport, uint16_t gpios){GPIO_BSRR(gpioport) = (gpios << 16);}
inline int  pin_read(uint32_t gpioport, uint16_t gpios){return GPIO_IDR(gpioport) & gpios ? 1 : 0;}
inline void pin_write(uint32_t gpioport, uint16_t gpios){GPIO_ODR(gpioport) = gpios;}

//#define  do{}while(0)



