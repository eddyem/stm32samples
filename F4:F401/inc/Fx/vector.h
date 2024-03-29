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

#if defined STM32F0
void WEAK wwdg_isr(void);
void WEAK pvd_isr(void);
void WEAK rtc_isr(void);
void WEAK flash_isr(void);
void WEAK rcc_isr(void);
void WEAK exti0_1_isr(void);
void WEAK exti2_3_isr(void);
void WEAK exti4_15_isr(void);
void WEAK tsc_isr(void);
void WEAK dma1_channel1_isr(void);
void WEAK dma1_channel2_3_isr(void);
void WEAK dma1_channel4_5_isr(void);
void WEAK adc_comp_isr(void);
void WEAK tim1_brk_up_trg_com_isr(void);
void WEAK tim1_cc_isr(void);
void WEAK tim2_isr(void);
void WEAK tim3_isr(void);
void WEAK tim6_dac_isr(void);
void WEAK tim7_isr(void);
void WEAK tim14_isr(void);
void WEAK tim15_isr(void);
void WEAK tim16_isr(void);
void WEAK tim17_isr(void);
void WEAK i2c1_isr(void);
void WEAK i2c2_isr(void);
void WEAK spi1_isr(void);
void WEAK spi2_isr(void);
void WEAK usart1_isr(void);
void WEAK usart2_isr(void);
void WEAK usart3_4_isr(void);
void WEAK cec_can_isr(void);
void WEAK usb_isr(void);

#elif defined STM32F1
void WEAK wwdg_isr(void);
void WEAK pvd_isr(void);
void WEAK tamper_isr(void);
void WEAK rtc_isr(void);
void WEAK flash_isr(void);
void WEAK rcc_isr(void);
void WEAK exti0_isr(void);
void WEAK exti1_isr(void);
void WEAK exti2_isr(void);
void WEAK exti3_isr(void);
void WEAK exti4_isr(void);
void WEAK dma1_channel1_isr(void);
void WEAK dma1_channel2_isr(void);
void WEAK dma1_channel3_isr(void);
void WEAK dma1_channel4_isr(void);
void WEAK dma1_channel5_isr(void);
void WEAK dma1_channel6_isr(void);
void WEAK dma1_channel7_isr(void);
void WEAK adc1_2_isr(void);
void WEAK usb_hp_can_tx_isr(void);
void WEAK usb_lp_can_rx0_isr(void);
void WEAK can_rx1_isr(void);
void WEAK can_sce_isr(void);
void WEAK exti9_5_isr(void);
void WEAK tim1_brk_isr(void);
void WEAK tim1_up_isr(void);
void WEAK tim1_trg_com_isr(void);
void WEAK tim1_cc_isr(void);
void WEAK tim2_isr(void);
void WEAK tim3_isr(void);
void WEAK tim4_isr(void);
void WEAK i2c1_ev_isr(void);
void WEAK i2c1_er_isr(void);
void WEAK i2c2_ev_isr(void);
void WEAK i2c2_er_isr(void);
void WEAK spi1_isr(void);
void WEAK spi2_isr(void);
void WEAK usart1_isr(void);
void WEAK usart2_isr(void);
void WEAK usart3_isr(void);
void WEAK exti15_10_isr(void);
void WEAK rtc_alarm_isr(void);
void WEAK usb_wakeup_isr(void);
void WEAK tim8_brk_isr(void);
void WEAK tim8_up_isr(void);
void WEAK tim8_trg_com_isr(void);
void WEAK tim8_cc_isr(void);
void WEAK adc3_isr(void);
void WEAK fsmc_isr(void);
void WEAK sdio_isr(void);
void WEAK tim5_isr(void);
void WEAK spi3_isr(void);
void WEAK uart4_isr(void);
void WEAK uart5_isr(void);
void WEAK tim6_isr(void);
void WEAK tim7_isr(void);
void WEAK dma2_channel1_isr(void);
void WEAK dma2_channel2_isr(void);
void WEAK dma2_channel3_isr(void);
void WEAK dma2_channel4_5_isr(void);
void WEAK dma2_channel5_isr(void);
void WEAK eth_isr(void);
void WEAK eth_wkup_isr(void);
void WEAK can2_tx_isr(void);
void WEAK can2_rx0_isr(void);
void WEAK can2_rx1_isr(void);
void WEAK can2_sce_isr(void);
void WEAK otg_fs_isr(void);

#elif defined STM32F2
void WEAK nvic_wwdg_isr(void);
void WEAK pvd_isr(void);
void WEAK tamp_stamp_isr(void);
void WEAK rtc_wkup_isr(void);
void WEAK flash_isr(void);
void WEAK rcc_isr(void);
void WEAK exti0_isr(void);
void WEAK exti1_isr(void);
void WEAK exti2_isr(void);
void WEAK exti3_isr(void);
void WEAK exti4_isr(void);
void WEAK dma1_stream0_isr(void);
void WEAK dma1_stream1_isr(void);
void WEAK dma1_stream2_isr(void);
void WEAK dma1_stream3_isr(void);
void WEAK dma1_stream4_isr(void);
void WEAK dma1_stream5_isr(void);
void WEAK dma1_stream6_isr(void);
void WEAK adc_isr(void);
void WEAK can1_tx_isr(void);
void WEAK can1_rx0_isr(void);
void WEAK can1_rx1_isr(void);
void WEAK can1_sce_isr(void);
void WEAK exti9_5_isr(void);
void WEAK tim1_brk_tim9_isr(void);
void WEAK tim1_up_tim10_isr(void);
void WEAK tim1_trg_com_tim11_isr(void);
void WEAK tim1_cc_isr(void);
void WEAK tim2_isr(void);
void WEAK tim3_isr(void);
void WEAK tim4_isr(void);
void WEAK i2c1_ev_isr(void);
void WEAK i2c1_er_isr(void);
void WEAK i2c2_ev_isr(void);
void WEAK i2c2_er_isr(void);
void WEAK spi1_isr(void);
void WEAK spi2_isr(void);
void WEAK usart1_isr(void);
void WEAK usart2_isr(void);
void WEAK usart3_isr(void);
void WEAK exti15_10_isr(void);
void WEAK rtc_alarm_isr(void);
void WEAK usb_fs_wkup_isr(void);
void WEAK tim8_brk_tim12_isr(void);
void WEAK tim8_up_tim13_isr(void);
void WEAK tim8_trg_com_tim14_isr(void);
void WEAK tim8_cc_isr(void);
void WEAK dma1_stream7_isr(void);
void WEAK fsmc_isr(void);
void WEAK sdio_isr(void);
void WEAK tim5_isr(void);
void WEAK spi3_isr(void);
void WEAK uart4_isr(void);
void WEAK uart5_isr(void);
void WEAK tim6_dac_isr(void);
void WEAK tim7_isr(void);
void WEAK dma2_stream0_isr(void);
void WEAK dma2_stream1_isr(void);
void WEAK dma2_stream2_isr(void);
void WEAK dma2_stream3_isr(void);
void WEAK dma2_stream4_isr(void);
void WEAK eth_isr(void);
void WEAK eth_wkup_isr(void);
void WEAK can2_tx_isr(void);
void WEAK can2_rx0_isr(void);
void WEAK can2_rx1_isr(void);
void WEAK can2_sce_isr(void);
void WEAK otg_fs_isr(void);
void WEAK dma2_stream5_isr(void);
void WEAK dma2_stream6_isr(void);
void WEAK dma2_stream7_isr(void);
void WEAK usart6_isr(void);
void WEAK i2c3_ev_isr(void);
void WEAK i2c3_er_isr(void);
void WEAK otg_hs_ep1_out_isr(void);
void WEAK otg_hs_ep1_in_isr(void);
void WEAK otg_hs_wkup_isr(void);
void WEAK otg_hs_isr(void);
void WEAK dcmi_isr(void);
void WEAK cryp_isr(void);
void WEAK hash_rng_isr(void);

#elif defined STM32F3
void WEAK nvic_wwdg_isr(void);
void WEAK pvd_isr(void);
void WEAK tamp_stamp_isr(void);
void WEAK rtc_wkup_isr(void);
void WEAK flash_isr(void);
void WEAK rcc_isr(void);
void WEAK exti0_isr(void);
void WEAK exti1_isr(void);
void WEAK exti2_tsc_isr(void);
void WEAK exti3_isr(void);
void WEAK exti4_isr(void);
void WEAK dma1_channel1_isr(void);
void WEAK dma1_channel2_isr(void);
void WEAK dma1_channel3_isr(void);
void WEAK dma1_channel4_isr(void);
void WEAK dma1_channel5_isr(void);
void WEAK dma1_channel6_isr(void);
void WEAK dma1_channel7_isr(void);
void WEAK adc1_2_isr(void);
void WEAK usb_hp_can1_tx_isr(void);
void WEAK usb_lp_can1_rx0_isr(void);
void WEAK can1_rx1_isr(void);
void WEAK can1_sce_isr(void);
void WEAK exti9_5_isr(void);
void WEAK tim1_brk_tim15_isr(void);
void WEAK tim1_up_tim16_isr(void);
void WEAK tim1_trg_com_tim17_isr(void);
void WEAK tim1_cc_isr(void);
void WEAK tim2_isr(void);
void WEAK tim3_isr(void);
void WEAK tim4_isr(void);
void WEAK i2c1_ev_exti23_isr(void);
void WEAK i2c1_er_isr(void);
void WEAK i2c2_ev_exti24_isr(void);
void WEAK i2c2_er_isr(void);
void WEAK spi1_isr(void);
void WEAK spi2_isr(void);
void WEAK usart1_exti25_isr(void);
void WEAK usart2_exti26_isr(void);
void WEAK usart3_exti28_isr(void);
void WEAK exti15_10_isr(void);
void WEAK rtc_alarm_isr(void);
void WEAK usb_wkup_a_isr(void);
void WEAK tim8_brk_isr(void);
void WEAK tim8_up_isr(void);
void WEAK tim8_trg_com_isr(void);
void WEAK tim8_cc_isr(void);
void WEAK adc3_isr(void);
void WEAK reserved_1_isr(void);
void WEAK reserved_2_isr(void);
void WEAK reserved_3_isr(void);
void WEAK spi3_isr(void);
void WEAK uart4_exti34_isr(void);
void WEAK uart5_exti35_isr(void);
void WEAK tim6_dac_isr(void);
void WEAK tim7_isr(void);
void WEAK dma2_channel1_isr(void);
void WEAK dma2_channel2_isr(void);
void WEAK dma2_channel3_isr(void);
void WEAK dma2_channel4_isr(void);
void WEAK dma2_channel5_isr(void);
void WEAK adc4_isr(void);
void WEAK reserved_4_isr(void);
void WEAK reserved_5_isr(void);
void WEAK comp123_isr(void);
void WEAK comp456_isr(void);
void WEAK comp7_isr(void);
void WEAK reserved_6_isr(void);
void WEAK reserved_7_isr(void);
void WEAK reserved_8_isr(void);
void WEAK reserved_9_isr(void);
void WEAK reserved_10_isr(void);
void WEAK reserved_11_isr(void);
void WEAK reserved_12_isr(void);
void WEAK usb_hp_isr(void);
void WEAK usb_lp_isr(void);
void WEAK usb_wkup_isr(void);
void WEAK reserved_13_isr(void);
void WEAK reserved_14_isr(void);
void WEAK reserved_15_isr(void);
void WEAK reserved_16_isr(void);
void WEAK fpu_isr(void);

#elif defined STM32F4
#include "stm32f4.h"
void WEAK nvic_wwdg_isr(void);
void WEAK pvd_isr(void);
void WEAK tamp_stamp_isr(void);
void WEAK rtc_wkup_isr(void);
void WEAK flash_isr(void);
void WEAK rcc_isr(void);
void WEAK exti0_isr(void);
void WEAK exti1_isr(void);
void WEAK exti2_isr(void);
void WEAK exti3_isr(void);
void WEAK exti4_isr(void);
void WEAK dma1_stream0_isr(void);
void WEAK dma1_stream1_isr(void);
void WEAK dma1_stream2_isr(void);
void WEAK dma1_stream3_isr(void);
void WEAK dma1_stream4_isr(void);
void WEAK dma1_stream5_isr(void);
void WEAK dma1_stream6_isr(void);
void WEAK adc_isr(void);
void WEAK can1_tx_isr(void);
void WEAK can1_rx0_isr(void);
void WEAK can1_rx1_isr(void);
void WEAK can1_sce_isr(void);
void WEAK exti9_5_isr(void);
void WEAK tim1_brk_tim9_isr(void);
void WEAK tim1_up_tim10_isr(void);
void WEAK tim1_trg_com_tim11_isr(void);
void WEAK tim1_cc_isr(void);
void WEAK tim2_isr(void);
void WEAK tim3_isr(void);
void WEAK tim4_isr(void);
void WEAK i2c1_ev_isr(void);
void WEAK i2c1_er_isr(void);
void WEAK i2c2_ev_isr(void);
void WEAK i2c2_er_isr(void);
void WEAK spi1_isr(void);
void WEAK spi2_isr(void);
void WEAK usart1_isr(void);
void WEAK usart2_isr(void);
void WEAK usart3_isr(void);
void WEAK exti15_10_isr(void);
void WEAK rtc_alarm_isr(void);
void WEAK usb_fs_wkup_isr(void);
void WEAK tim8_brk_tim12_isr(void);
void WEAK tim8_up_tim13_isr(void);
void WEAK tim8_trg_com_tim14_isr(void);
void WEAK tim8_cc_isr(void);
void WEAK dma1_stream7_isr(void);
void WEAK fsmc_isr(void);
void WEAK sdio_isr(void);
void WEAK tim5_isr(void);
void WEAK spi3_isr(void);
void WEAK uart4_isr(void);
void WEAK uart5_isr(void);
void WEAK tim6_dac_isr(void);
void WEAK tim7_isr(void);
void WEAK dma2_stream0_isr(void);
void WEAK dma2_stream1_isr(void);
void WEAK dma2_stream2_isr(void);
void WEAK dma2_stream3_isr(void);
void WEAK dma2_stream4_isr(void);
void WEAK eth_isr(void);
void WEAK eth_wkup_isr(void);
void WEAK can2_tx_isr(void);
void WEAK can2_rx0_isr(void);
void WEAK can2_rx1_isr(void);
void WEAK can2_sce_isr(void);
void WEAK otg_fs_isr(void);
void WEAK dma2_stream5_isr(void);
void WEAK dma2_stream6_isr(void);
void WEAK dma2_stream7_isr(void);
void WEAK usart6_isr(void);
void WEAK i2c3_ev_isr(void);
void WEAK i2c3_er_isr(void);
void WEAK otg_hs_ep1_out_isr(void);
void WEAK otg_hs_ep1_in_isr(void);
void WEAK otg_hs_wkup_isr(void);
void WEAK otg_hs_isr(void);
void WEAK dcmi_isr(void);
void WEAK cryp_isr(void);
void WEAK hash_rng_isr(void);
void WEAK fpu_isr(void);
void WEAK uart7_isr(void);
void WEAK uart8_isr(void);
void WEAK spi4_isr(void);
void WEAK spi5_isr(void);
void WEAK spi6_isr(void);
void WEAK sai1_isr(void);
void WEAK lcd_tft_isr(void);
void WEAK lcd_tft_err_isr(void);
void WEAK dma2d_isr(void);

#else
    #error "Not supported platform"
#endif

#endif // VECTOR_H
