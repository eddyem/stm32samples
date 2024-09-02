/*
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "usb.h"
#include "usb_lib.h"

// here we suppose that all PIN settings done in hw_setup earlier
void USB_setup(){
#if defined STM32F3
    NVIC_DisableIRQ(USB_LP_IRQn);
    // remap USB LP & Wakeup interrupts to 75 and 76 - works only on pure F303
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // enable tacting of SYSCFG
    SYSCFG->CFGR1 |= SYSCFG_CFGR1_USB_IT_RMP;
#elif defined STM32F1
    NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
    NVIC_DisableIRQ(USB_HP_CAN1_TX_IRQn);
#elif defined STM32F0
    NVIC_DisableIRQ(USB_IRQn);
    RCC->APB1ENR |= RCC_APB1ENR_CRSEN;
    RCC->CFGR3 &= ~RCC_CFGR3_USBSW; // reset USB
    RCC->CR2 |= RCC_CR2_HSI48ON; // turn ON HSI48
    uint32_t tmout = 16000000;
    while(!(RCC->CR2 & RCC_CR2_HSI48RDY)){if(--tmout == 0) break;}
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;
    CRS->CFGR &= ~CRS_CFGR_SYNCSRC;
    CRS->CFGR |= CRS_CFGR_SYNCSRC_1; // USB SOF selected as sync source
    CRS->CR |= CRS_CR_AUTOTRIMEN; // enable auto trim
    CRS->CR |= CRS_CR_CEN; // enable freq counter & block CRS->CFGR as read-only
    RCC->CFGR |= RCC_CFGR_SW;
#endif
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;
    //??
    USB->CNTR   = USB_CNTR_FRES; // Force USB Reset
    for(uint32_t ctr = 0; ctr < 72000; ++ctr) nop(); // wait >1ms
    USB->CNTR   = 0;
    USB->BTABLE = 0;
    USB->DADDR  = 0;
    USB->ISTR   = 0;
    USB->CNTR   = USB_CNTR_RESETM | USB_CNTR_WKUPM; // allow only wakeup & reset interrupts
#if defined STM32F3
    NVIC_EnableIRQ(USB_LP_IRQn);
#elif defined STM32F1
    NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
#elif defined STM32F0
    USB->BCDR |= USB_BCDR_DPPU;
    NVIC_EnableIRQ(USB_IRQn);
#endif
}


