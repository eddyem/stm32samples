/*
 * vector.h
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
#pragma once
#ifndef VECTOR_H
#define VECTOR_H 

#ifndef WEAK
#define WEAK __attribute__((weak))
#endif

void WEAK reset_handler(void);
void WEAK nmi_handler(void);
void WEAK hard_fault_handler(void);
void WEAK sv_call_handler(void);
void WEAK pend_sv_handler(void);
void WEAK sys_tick_handler(void);

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
void WEAK mem_manage_handler(void);
void WEAK bus_fault_handler(void);
void WEAK usage_fault_handler(void);
void WEAK debug_monitor_handler(void);
#endif

#if defined STM32G0
void WEAK wwdg_isr(void);
void WEAK rtc_isr(void);
void WEAK flash_isr(void);
void WEAK rcc_isr(void);
void WEAK exti0_1_isr(void);
void WEAK exti2_3_isr(void);
void WEAK exti4_15_isr(void);
void WEAK dma1_channel1_isr(void);
void WEAK dma1_channel2_3_isr(void);
void WEAK dmamux_isr(void);
void WEAK adc_comp_isr(void);
void WEAK tim1_brk_up_trg_com_isr(void);
void WEAK tim1_cc_isr(void);
void WEAK tim3_4_isr(void);
void WEAK tim6_dac_isr(void);
void WEAK tim7_isr(void);
void WEAK tim14_isr(void);
void WEAK tim15_isr(void);
void WEAK tim16_isr(void);
void WEAK tim17_isr(void);
void WEAK i2c1_isr(void);
void WEAK i2c2_3_isr(void);
void WEAK spi1_isr(void);
void WEAK spi2_3_isr(void);
void WEAK usart1_isr(void);
void WEAK usart2_isr(void);
void WEAK usart3_6_isr(void);
void WEAK cec_can_isr(void);
void WEAK usb_isr(void);
#else
    #error "Not supported platform"
#endif

#endif // VECTOR_H
