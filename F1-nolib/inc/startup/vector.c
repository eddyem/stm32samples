/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Piotr Esden-Tempski <piotr@esden.net>,
 * Copyright (C) 2012 chrysn <chrysn@fsfe.org>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>.
 */
#include "vector.h"

/* Initialization template for the interrupt vector table. This definition is
 * used by the startup code generator (vector.c) to set the initial values for
 * the interrupt handling routines to the chip family specific _isr weak
 * symbols. */
#if defined STM32F0
#include "stm32f0xx.h"
#define NVIC_IRQ_COUNT 32
#define IRQ_HANDLERS \
    wwdg_isr, \
    pvd_isr, \
    rtc_isr, \
    flash_isr, \
    rcc_isr, \
    exti0_1_isr, \
    exti2_3_isr, \
    exti4_15_isr, \
    tsc_isr, \
    dma1_channel1_isr, \
    dma1_channel2_3_isr, \
    dma1_channel4_5_isr, \
    adc_comp_isr, \
    tim1_brk_up_trg_com_isr, \
    tim1_cc_isr, \
    tim2_isr, \
    tim3_isr, \
    tim6_dac_isr, \
    tim7_isr, \
    tim14_isr, \
    tim15_isr, \
    tim16_isr, \
    tim17_isr, \
    i2c1_isr, \
    i2c2_isr, \
    spi1_isr, \
    spi2_isr, \
    usart1_isr, \
    usart2_isr, \
    usart3_4_isr, \
    cec_can_isr, \
    usb_isr
#elif defined STM32F1
#include "stm32f10x.h"
#define NVIC_IRQ_COUNT 68
#define IRQ_HANDLERS \
    wwdg_isr, \
    pvd_isr, \
    tamper_isr, \
    rtc_isr, \
    flash_isr, \
    rcc_isr, \
    exti0_isr, \
    exti1_isr, \
    exti2_isr, \
    exti3_isr, \
    exti4_isr, \
    dma1_channel1_isr, \
    dma1_channel2_isr, \
    dma1_channel3_isr, \
    dma1_channel4_isr, \
    dma1_channel5_isr, \
    dma1_channel6_isr, \
    dma1_channel7_isr, \
    adc1_2_isr, \
    usb_hp_can_tx_isr, \
    usb_lp_can_rx0_isr, \
    can_rx1_isr, \
    can_sce_isr, \
    exti9_5_isr, \
    tim1_brk_isr, \
    tim1_up_isr, \
    tim1_trg_com_isr, \
    tim1_cc_isr, \
    tim2_isr, \
    tim3_isr, \
    tim4_isr, \
    i2c1_ev_isr, \
    i2c1_er_isr, \
    i2c2_ev_isr, \
    i2c2_er_isr, \
    spi1_isr, \
    spi2_isr, \
    usart1_isr, \
    usart2_isr, \
    usart3_isr, \
    exti15_10_isr, \
    rtc_alarm_isr, \
    usb_wakeup_isr, \
    tim8_brk_isr, \
    tim8_up_isr, \
    tim8_trg_com_isr, \
    tim8_cc_isr, \
    adc3_isr, \
    fsmc_isr, \
    sdio_isr, \
    tim5_isr, \
    spi3_isr, \
    uart4_isr, \
    uart5_isr, \
    tim6_isr, \
    tim7_isr, \
    dma2_channel1_isr, \
    dma2_channel2_isr, \
    dma2_channel3_isr, \
    dma2_channel4_5_isr, \
    dma2_channel5_isr, \
    eth_isr, \
    eth_wkup_isr, \
    can2_tx_isr, \
    can2_rx0_isr, \
    can2_rx1_isr, \
    can2_sce_isr, \
    otg_fs_isr
#elif defined STM32F2
    #define NVIC_IRQ_COUNT 81
    #define IRQ_HANDLERS \
    nvic_wwdg_isr, \
    pvd_isr, \
    tamp_stamp_isr, \
    rtc_wkup_isr, \
    flash_isr, \
    rcc_isr, \
    exti0_isr, \
    exti1_isr, \
    exti2_isr, \
    exti3_isr, \
    exti4_isr, \
    dma1_stream0_isr, \
    dma1_stream1_isr, \
    dma1_stream2_isr, \
    dma1_stream3_isr, \
    dma1_stream4_isr, \
    dma1_stream5_isr, \
    dma1_stream6_isr, \
    adc_isr, \
    can1_tx_isr, \
    can1_rx0_isr, \
    can1_rx1_isr, \
    can1_sce_isr, \
    exti9_5_isr, \
    tim1_brk_tim9_isr, \
    tim1_up_tim10_isr, \
    tim1_trg_com_tim11_isr, \
    tim1_cc_isr, \
    tim2_isr, \
    tim3_isr, \
    tim4_isr, \
    i2c1_ev_isr, \
    i2c1_er_isr, \
    i2c2_ev_isr, \
    i2c2_er_isr, \
    spi1_isr, \
    spi2_isr, \
    usart1_isr, \
    usart2_isr, \
    usart3_isr, \
    exti15_10_isr, \
    rtc_alarm_isr, \
    usb_fs_wkup_isr, \
    tim8_brk_tim12_isr, \
    tim8_up_tim13_isr, \
    tim8_trg_com_tim14_isr, \
    tim8_cc_isr, \
    dma1_stream7_isr, \
    fsmc_isr, \
    sdio_isr, \
    tim5_isr, \
    spi3_isr, \
    uart4_isr, \
    uart5_isr, \
    tim6_dac_isr, \
    tim7_isr, \
    dma2_stream0_isr, \
    dma2_stream1_isr, \
    dma2_stream2_isr, \
    dma2_stream3_isr, \
    dma2_stream4_isr, \
    eth_isr, \
    eth_wkup_isr, \
    can2_tx_isr, \
    can2_rx0_isr, \
    can2_rx1_isr, \
    can2_sce_isr, \
    otg_fs_isr, \
    dma2_stream5_isr, \
    dma2_stream6_isr, \
    dma2_stream7_isr, \
    usart6_isr, \
    i2c3_ev_isr, \
    i2c3_er_isr, \
    otg_hs_ep1_out_isr, \
    otg_hs_ep1_in_isr, \
    otg_hs_wkup_isr, \
    otg_hs_isr, \
    dcmi_isr, \
    cryp_isr, \
    hash_rng_isr

#elif defined STM32F3
#define NVIC_IRQ_COUNT 81
#define IRQ_HANDLERS \
    nvic_wwdg_isr, \
    pvd_isr, \
    tamp_stamp_isr, \
    rtc_wkup_isr, \
    flash_isr, \
    rcc_isr, \
    exti0_isr, \
    exti1_isr, \
    exti2_tsc_isr, \
    exti3_isr, \
    exti4_isr, \
    dma1_channel1_isr, \
    dma1_channel2_isr, \
    dma1_channel3_isr, \
    dma1_channel4_isr, \
    dma1_channel5_isr, \
    dma1_channel6_isr, \
    dma1_channel7_isr, \
    adc1_2_isr, \
    usb_hp_can1_tx_isr, \
    usb_lp_can1_rx0_isr, \
    can1_rx1_isr, \
    can1_sce_isr, \
    exti9_5_isr, \
    tim1_brk_tim15_isr, \
    tim1_up_tim16_isr, \
    tim1_trg_com_tim17_isr, \
    tim1_cc_isr, \
    tim2_isr, \
    tim3_isr, \
    tim4_isr, \
    i2c1_ev_exti23_isr, \
    i2c1_er_isr, \
    i2c2_ev_exti24_isr, \
    i2c2_er_isr, \
    spi1_isr, \
    spi2_isr, \
    usart1_exti25_isr, \
    usart2_exti26_isr, \
    usart3_exti28_isr, \
    exti15_10_isr, \
    rtc_alarm_isr, \
    usb_wkup_a_isr, \
    tim8_brk_isr, \
    tim8_up_isr, \
    tim8_trg_com_isr, \
    tim8_cc_isr, \
    adc3_isr, \
    reserved_1_isr, \
    reserved_2_isr, \
    reserved_3_isr, \
    spi3_isr, \
    uart4_exti34_isr, \
    uart5_exti35_isr, \
    tim6_dac_isr, \
    tim7_isr, \
    dma2_channel1_isr, \
    dma2_channel2_isr, \
    dma2_channel3_isr, \
    dma2_channel4_isr, \
    dma2_channel5_isr, \
    eth_isr, \
    reserved_4_isr, \
    reserved_5_isr, \
    comp123_isr, \
    comp456_isr, \
    comp7_isr, \
    reserved_6_isr, \
    reserved_7_isr, \
    reserved_8_isr, \
    reserved_9_isr, \
    reserved_10_isr, \
    reserved_11_isr, \
    reserved_12_isr, \
    usb_hp_isr, \
    usb_lp_isr, \
    usb_wkup_isr, \
    reserved_13_isr, \
    reserved_14_isr, \
    reserved_15_isr, \
    reserved_16_isr

#elif defined STM32F4
#define NVIC_IRQ_COUNT 91
    #define IRQ_HANDLERS \
    nvic_wwdg_isr, \
    pvd_isr, \
    tamp_stamp_isr, \
    rtc_wkup_isr, \
    flash_isr, \
    rcc_isr, \
    exti0_isr, \
    exti1_isr, \
    exti2_isr, \
    exti3_isr, \
    exti4_isr, \
    dma1_stream0_isr, \
    dma1_stream1_isr, \
    dma1_stream2_isr, \
    dma1_stream3_isr, \
    dma1_stream4_isr, \
    dma1_stream5_isr, \
    dma1_stream6_isr, \
    adc_isr, \
    can1_tx_isr, \
    can1_rx0_isr, \
    can1_rx1_isr, \
    can1_sce_isr, \
    exti9_5_isr, \
    tim1_brk_tim9_isr, \
    tim1_up_tim10_isr, \
    tim1_trg_com_tim11_isr, \
    tim1_cc_isr, \
    tim2_isr, \
    tim3_isr, \
    tim4_isr, \
    i2c1_ev_isr, \
    i2c1_er_isr, \
    i2c2_ev_isr, \
    i2c2_er_isr, \
    spi1_isr, \
    spi2_isr, \
    usart1_isr, \
    usart2_isr, \
    usart3_isr, \
    exti15_10_isr, \
    rtc_alarm_isr, \
    usb_fs_wkup_isr, \
    tim8_brk_tim12_isr, \
    tim8_up_tim13_isr, \
    tim8_trg_com_tim14_isr, \
    tim8_cc_isr, \
    dma1_stream7_isr, \
    fsmc_isr, \
    sdio_isr, \
    tim5_isr, \
    spi3_isr, \
    uart4_isr, \
    uart5_isr, \
    tim6_dac_isr, \
    tim7_isr, \
    dma2_stream0_isr, \
    dma2_stream1_isr, \
    dma2_stream2_isr, \
    dma2_stream3_isr, \
    dma2_stream4_isr, \
    eth_isr, \
    eth_wkup_isr, \
    can2_tx_isr, \
    can2_rx0_isr, \
    can2_rx1_isr, \
    can2_sce_isr, \
    otg_fs_isr, \
    dma2_stream5_isr, \
    dma2_stream6_isr, \
    dma2_stream7_isr, \
    usart6_isr, \
    i2c3_ev_isr, \
    i2c3_er_isr, \
    otg_hs_ep1_out_isr, \
    otg_hs_ep1_in_isr, \
    otg_hs_wkup_isr, \
    otg_hs_isr, \
    dcmi_isr, \
    cryp_isr, \
    hash_rng_isr, \
    fpu_isr, \
    uart7_isr, \
    uart8_isr, \
    spi4_isr, \
    spi5_isr, \
    spi6_isr, \
    sai1_isr, \
    lcd_tft_isr, \
    lcd_tft_err_isr, \
    dma2d_isr
#else 
    #error "Not supported STM32 family"
#endif

    #if defined STM32F0
#pragma weak nmi_handler = null_handler
#pragma weak hard_fault_handler = blocking_handler
#pragma weak sv_call_handler = null_handler
#pragma weak pend_sv_handler = null_handler
#pragma weak sys_tick_handler = null_handler

#pragma weak wwdg_isr = blocking_handler
#pragma weak pvd_isr = blocking_handler
#pragma weak rtc_isr = blocking_handler
#pragma weak flash_isr = blocking_handler
#pragma weak rcc_isr = blocking_handler
#pragma weak exti0_1_isr = blocking_handler
#pragma weak exti2_3_isr = blocking_handler
#pragma weak exti4_15_isr = blocking_handler
#pragma weak tsc_isr = blocking_handler
#pragma weak dma1_channel1_isr = blocking_handler
#pragma weak dma1_channel2_3_isr = blocking_handler
#pragma weak dma1_channel4_5_isr = blocking_handler
#pragma weak adc_comp_isr = blocking_handler
#pragma weak tim1_brk_up_trg_com_isr = blocking_handler
#pragma weak tim1_cc_isr = blocking_handler
#pragma weak tim2_isr = blocking_handler
#pragma weak tim3_isr = blocking_handler
#pragma weak tim6_dac_isr = blocking_handler
#pragma weak tim7_isr = blocking_handler
#pragma weak tim14_isr = blocking_handler
#pragma weak tim15_isr = blocking_handler
#pragma weak tim16_isr = blocking_handler
#pragma weak tim17_isr = blocking_handler
#pragma weak i2c1_isr = blocking_handler
#pragma weak i2c2_isr = blocking_handler
#pragma weak spi1_isr = blocking_handler
#pragma weak spi2_isr = blocking_handler
#pragma weak usart1_isr = blocking_handler
#pragma weak usart2_isr = blocking_handler
#pragma weak usart3_4_isr = blocking_handler
#pragma weak cec_can_isr = blocking_handler
#pragma weak usb_isr = blocking_handler

#elif defined STM32F1
#pragma weak wwdg_isr = blocking_handler
#pragma weak pvd_isr = blocking_handler
#pragma weak tamper_isr = blocking_handler
#pragma weak rtc_isr = blocking_handler
#pragma weak flash_isr = blocking_handler
#pragma weak rcc_isr = blocking_handler
#pragma weak exti0_isr = blocking_handler
#pragma weak exti1_isr = blocking_handler
#pragma weak exti2_isr = blocking_handler
#pragma weak exti3_isr = blocking_handler
#pragma weak exti4_isr = blocking_handler
#pragma weak dma1_channel1_isr = blocking_handler
#pragma weak dma1_channel2_isr = blocking_handler
#pragma weak dma1_channel3_isr = blocking_handler
#pragma weak dma1_channel4_isr = blocking_handler
#pragma weak dma1_channel5_isr = blocking_handler
#pragma weak dma1_channel6_isr = blocking_handler
#pragma weak dma1_channel7_isr = blocking_handler
#pragma weak adc1_2_isr = blocking_handler
#pragma weak usb_hp_can_tx_isr = blocking_handler
#pragma weak usb_lp_can_rx0_isr = blocking_handler
#pragma weak can_rx1_isr = blocking_handler
#pragma weak can_sce_isr = blocking_handler
#pragma weak exti9_5_isr = blocking_handler
#pragma weak tim1_brk_isr = blocking_handler
#pragma weak tim1_up_isr = blocking_handler
#pragma weak tim1_trg_com_isr = blocking_handler
#pragma weak tim1_cc_isr = blocking_handler
#pragma weak tim2_isr = blocking_handler
#pragma weak tim3_isr = blocking_handler
#pragma weak tim4_isr = blocking_handler
#pragma weak i2c1_ev_isr = blocking_handler
#pragma weak i2c1_er_isr = blocking_handler
#pragma weak i2c2_ev_isr = blocking_handler
#pragma weak i2c2_er_isr = blocking_handler
#pragma weak spi1_isr = blocking_handler
#pragma weak spi2_isr = blocking_handler
#pragma weak usart1_isr = blocking_handler
#pragma weak usart2_isr = blocking_handler
#pragma weak usart3_isr = blocking_handler
#pragma weak exti15_10_isr = blocking_handler
#pragma weak rtc_alarm_isr = blocking_handler
#pragma weak usb_wakeup_isr = blocking_handler
#pragma weak tim8_brk_isr = blocking_handler
#pragma weak tim8_up_isr = blocking_handler
#pragma weak tim8_trg_com_isr = blocking_handler
#pragma weak tim8_cc_isr = blocking_handler
#pragma weak adc3_isr = blocking_handler
#pragma weak fsmc_isr = blocking_handler
#pragma weak sdio_isr = blocking_handler
#pragma weak tim5_isr = blocking_handler
#pragma weak spi3_isr = blocking_handler
#pragma weak uart4_isr = blocking_handler
#pragma weak uart5_isr = blocking_handler
#pragma weak tim6_isr = blocking_handler
#pragma weak tim7_isr = blocking_handler
#pragma weak dma2_channel1_isr = blocking_handler
#pragma weak dma2_channel2_isr = blocking_handler
#pragma weak dma2_channel3_isr = blocking_handler
#pragma weak dma2_channel4_5_isr = blocking_handler
#pragma weak dma2_channel5_isr = blocking_handler
#pragma weak eth_isr = blocking_handler
#pragma weak eth_wkup_isr = blocking_handler
#pragma weak can2_tx_isr = blocking_handler
#pragma weak can2_rx0_isr = blocking_handler
#pragma weak can2_rx1_isr = blocking_handler
#pragma weak can2_sce_isr = blocking_handler
#pragma weak otg_fs_isr = blocking_handler

#elif defined STM32F2
#pragma weak nvic_wwdg_isr = blocking_handler
#pragma weak pvd_isr = blocking_handler
#pragma weak tamp_stamp_isr = blocking_handler
#pragma weak rtc_wkup_isr = blocking_handler
#pragma weak flash_isr = blocking_handler
#pragma weak rcc_isr = blocking_handler
#pragma weak exti0_isr = blocking_handler
#pragma weak exti1_isr = blocking_handler
#pragma weak exti2_isr = blocking_handler
#pragma weak exti3_isr = blocking_handler
#pragma weak exti4_isr = blocking_handler
#pragma weak dma1_stream0_isr = blocking_handler
#pragma weak dma1_stream1_isr = blocking_handler
#pragma weak dma1_stream2_isr = blocking_handler
#pragma weak dma1_stream3_isr = blocking_handler
#pragma weak dma1_stream4_isr = blocking_handler
#pragma weak dma1_stream5_isr = blocking_handler
#pragma weak dma1_stream6_isr = blocking_handler
#pragma weak adc_isr = blocking_handler
#pragma weak can1_tx_isr = blocking_handler
#pragma weak can1_rx0_isr = blocking_handler
#pragma weak can1_rx1_isr = blocking_handler
#pragma weak can1_sce_isr = blocking_handler
#pragma weak exti9_5_isr = blocking_handler
#pragma weak tim1_brk_tim9_isr = blocking_handler
#pragma weak tim1_up_tim10_isr = blocking_handler
#pragma weak tim1_trg_com_tim11_isr = blocking_handler
#pragma weak tim1_cc_isr = blocking_handler
#pragma weak tim2_isr = blocking_handler
#pragma weak tim3_isr = blocking_handler
#pragma weak tim4_isr = blocking_handler
#pragma weak i2c1_ev_isr = blocking_handler
#pragma weak i2c1_er_isr = blocking_handler
#pragma weak i2c2_ev_isr = blocking_handler
#pragma weak i2c2_er_isr = blocking_handler
#pragma weak spi1_isr = blocking_handler
#pragma weak spi2_isr = blocking_handler
#pragma weak usart1_isr = blocking_handler
#pragma weak usart2_isr = blocking_handler
#pragma weak usart3_isr = blocking_handler
#pragma weak exti15_10_isr = blocking_handler
#pragma weak rtc_alarm_isr = blocking_handler
#pragma weak usb_fs_wkup_isr = blocking_handler
#pragma weak tim8_brk_tim12_isr = blocking_handler
#pragma weak tim8_up_tim13_isr = blocking_handler
#pragma weak tim8_trg_com_tim14_isr = blocking_handler
#pragma weak tim8_cc_isr = blocking_handler
#pragma weak dma1_stream7_isr = blocking_handler
#pragma weak fsmc_isr = blocking_handler
#pragma weak sdio_isr = blocking_handler
#pragma weak tim5_isr = blocking_handler
#pragma weak spi3_isr = blocking_handler
#pragma weak uart4_isr = blocking_handler
#pragma weak uart5_isr = blocking_handler
#pragma weak tim6_dac_isr = blocking_handler
#pragma weak tim7_isr = blocking_handler
#pragma weak dma2_stream0_isr = blocking_handler
#pragma weak dma2_stream1_isr = blocking_handler
#pragma weak dma2_stream2_isr = blocking_handler
#pragma weak dma2_stream3_isr = blocking_handler
#pragma weak dma2_stream4_isr = blocking_handler
#pragma weak eth_isr = blocking_handler
#pragma weak eth_wkup_isr = blocking_handler
#pragma weak can2_tx_isr = blocking_handler
#pragma weak can2_rx0_isr = blocking_handler
#pragma weak can2_rx1_isr = blocking_handler
#pragma weak can2_sce_isr = blocking_handler
#pragma weak otg_fs_isr = blocking_handler
#pragma weak dma2_stream5_isr = blocking_handler
#pragma weak dma2_stream6_isr = blocking_handler
#pragma weak dma2_stream7_isr = blocking_handler
#pragma weak usart6_isr = blocking_handler
#pragma weak i2c3_ev_isr = blocking_handler
#pragma weak i2c3_er_isr = blocking_handler
#pragma weak otg_hs_ep1_out_isr = blocking_handler
#pragma weak otg_hs_ep1_in_isr = blocking_handler
#pragma weak otg_hs_wkup_isr = blocking_handler
#pragma weak otg_hs_isr = blocking_handler
#pragma weak dcmi_isr = blocking_handler
#pragma weak cryp_isr = blocking_handler
#pragma weak hash_rng_isr = blocking_handler

#elif defined STM32F3
#pragma weak nvic_wwdg_isr = blocking_handler
#pragma weak pvd_isr = blocking_handler
#pragma weak tamp_stamp_isr = blocking_handler
#pragma weak rtc_wkup_isr = blocking_handler
#pragma weak flash_isr = blocking_handler
#pragma weak rcc_isr = blocking_handler
#pragma weak exti0_isr = blocking_handler
#pragma weak exti1_isr = blocking_handler
#pragma weak exti2_tsc_isr = blocking_handler
#pragma weak exti3_isr = blocking_handler
#pragma weak exti4_isr = blocking_handler
#pragma weak dma1_channel1_isr = blocking_handler
#pragma weak dma1_channel2_isr = blocking_handler
#pragma weak dma1_channel3_isr = blocking_handler
#pragma weak dma1_channel4_isr = blocking_handler
#pragma weak dma1_channel5_isr = blocking_handler
#pragma weak dma1_channel6_isr = blocking_handler
#pragma weak dma1_channel7_isr = blocking_handler
#pragma weak adc1_2_isr = blocking_handler
#pragma weak usb_hp_can1_tx_isr = blocking_handler
#pragma weak usb_lp_can1_rx0_isr = blocking_handler
#pragma weak can1_rx1_isr = blocking_handler
#pragma weak can1_sce_isr = blocking_handler
#pragma weak exti9_5_isr = blocking_handler
#pragma weak tim1_brk_tim15_isr = blocking_handler
#pragma weak tim1_up_tim16_isr = blocking_handler
#pragma weak tim1_trg_com_tim17_isr = blocking_handler
#pragma weak tim1_cc_isr = blocking_handler
#pragma weak tim2_isr = blocking_handler
#pragma weak tim3_isr = blocking_handler
#pragma weak tim4_isr = blocking_handler
#pragma weak i2c1_ev_exti23_isr = blocking_handler
#pragma weak i2c1_er_isr = blocking_handler
#pragma weak i2c2_ev_exti24_isr = blocking_handler
#pragma weak i2c2_er_isr = blocking_handler
#pragma weak spi1_isr = blocking_handler
#pragma weak spi2_isr = blocking_handler
#pragma weak usart1_exti25_isr = blocking_handler
#pragma weak usart2_exti26_isr = blocking_handler
#pragma weak usart3_exti28_isr = blocking_handler
#pragma weak exti15_10_isr = blocking_handler
#pragma weak rtc_alarm_isr = blocking_handler
#pragma weak usb_wkup_a_isr = blocking_handler
#pragma weak tim8_brk_isr = blocking_handler
#pragma weak tim8_up_isr = blocking_handler
#pragma weak tim8_trg_com_isr = blocking_handler
#pragma weak tim8_cc_isr = blocking_handler
#pragma weak adc3_isr = blocking_handler
#pragma weak reserved_1_isr = blocking_handler
#pragma weak reserved_2_isr = blocking_handler
#pragma weak reserved_3_isr = blocking_handler
#pragma weak spi3_isr = blocking_handler
#pragma weak uart4_exti34_isr = blocking_handler
#pragma weak uart5_exti35_isr = blocking_handler
#pragma weak tim6_dac_isr = blocking_handler
#pragma weak tim7_isr = blocking_handler
#pragma weak dma2_channel1_isr = blocking_handler
#pragma weak dma2_channel2_isr = blocking_handler
#pragma weak dma2_channel3_isr = blocking_handler
#pragma weak dma2_channel4_isr = blocking_handler
#pragma weak dma2_channel5_isr = blocking_handler
#pragma weak eth_isr = blocking_handler
#pragma weak reserved_4_isr = blocking_handler
#pragma weak reserved_5_isr = blocking_handler
#pragma weak comp123_isr = blocking_handler
#pragma weak comp456_isr = blocking_handler
#pragma weak comp7_isr = blocking_handler
#pragma weak reserved_6_isr = blocking_handler
#pragma weak reserved_7_isr = blocking_handler
#pragma weak reserved_8_isr = blocking_handler
#pragma weak reserved_9_isr = blocking_handler
#pragma weak reserved_10_isr = blocking_handler
#pragma weak reserved_11_isr = blocking_handler
#pragma weak reserved_12_isr = blocking_handler
#pragma weak usb_hp_isr = blocking_handler
#pragma weak usb_lp_isr = blocking_handler
#pragma weak usb_wkup_isr = blocking_handler
#pragma weak reserved_13_isr = blocking_handler
#pragma weak reserved_14_isr = blocking_handler
#pragma weak reserved_15_isr = blocking_handler
#pragma weak reserved_16_isr = blocking_handler

#elif defined STM32F4
#pragma weak nvic_wwdg_isr = blocking_handler
#pragma weak pvd_isr = blocking_handler
#pragma weak tamp_stamp_isr = blocking_handler
#pragma weak rtc_wkup_isr = blocking_handler
#pragma weak flash_isr = blocking_handler
#pragma weak rcc_isr = blocking_handler
#pragma weak exti0_isr = blocking_handler
#pragma weak exti1_isr = blocking_handler
#pragma weak exti2_isr = blocking_handler
#pragma weak exti3_isr = blocking_handler
#pragma weak exti4_isr = blocking_handler
#pragma weak dma1_stream0_isr = blocking_handler
#pragma weak dma1_stream1_isr = blocking_handler
#pragma weak dma1_stream2_isr = blocking_handler
#pragma weak dma1_stream3_isr = blocking_handler
#pragma weak dma1_stream4_isr = blocking_handler
#pragma weak dma1_stream5_isr = blocking_handler
#pragma weak dma1_stream6_isr = blocking_handler
#pragma weak adc_isr = blocking_handler
#pragma weak can1_tx_isr = blocking_handler
#pragma weak can1_rx0_isr = blocking_handler
#pragma weak can1_rx1_isr = blocking_handler
#pragma weak can1_sce_isr = blocking_handler
#pragma weak exti9_5_isr = blocking_handler
#pragma weak tim1_brk_tim9_isr = blocking_handler
#pragma weak tim1_up_tim10_isr = blocking_handler
#pragma weak tim1_trg_com_tim11_isr = blocking_handler
#pragma weak tim1_cc_isr = blocking_handler
#pragma weak tim2_isr = blocking_handler
#pragma weak tim3_isr = blocking_handler
#pragma weak tim4_isr = blocking_handler
#pragma weak i2c1_ev_isr = blocking_handler
#pragma weak i2c1_er_isr = blocking_handler
#pragma weak i2c2_ev_isr = blocking_handler
#pragma weak i2c2_er_isr = blocking_handler
#pragma weak spi1_isr = blocking_handler
#pragma weak spi2_isr = blocking_handler
#pragma weak usart1_isr = blocking_handler
#pragma weak usart2_isr = blocking_handler
#pragma weak usart3_isr = blocking_handler
#pragma weak exti15_10_isr = blocking_handler
#pragma weak rtc_alarm_isr = blocking_handler
#pragma weak usb_fs_wkup_isr = blocking_handler
#pragma weak tim8_brk_tim12_isr = blocking_handler
#pragma weak tim8_up_tim13_isr = blocking_handler
#pragma weak tim8_trg_com_tim14_isr = blocking_handler
#pragma weak tim8_cc_isr = blocking_handler
#pragma weak dma1_stream7_isr = blocking_handler
#pragma weak fsmc_isr = blocking_handler
#pragma weak sdio_isr = blocking_handler
#pragma weak tim5_isr = blocking_handler
#pragma weak spi3_isr = blocking_handler
#pragma weak uart4_isr = blocking_handler
#pragma weak uart5_isr = blocking_handler
#pragma weak tim6_dac_isr = blocking_handler
#pragma weak tim7_isr = blocking_handler
#pragma weak dma2_stream0_isr = blocking_handler
#pragma weak dma2_stream1_isr = blocking_handler
#pragma weak dma2_stream2_isr = blocking_handler
#pragma weak dma2_stream3_isr = blocking_handler
#pragma weak dma2_stream4_isr = blocking_handler
#pragma weak eth_isr = blocking_handler
#pragma weak eth_wkup_isr = blocking_handler
#pragma weak can2_tx_isr = blocking_handler
#pragma weak can2_rx0_isr = blocking_handler
#pragma weak can2_rx1_isr = blocking_handler
#pragma weak can2_sce_isr = blocking_handler
#pragma weak otg_fs_isr = blocking_handler
#pragma weak dma2_stream5_isr = blocking_handler
#pragma weak dma2_stream6_isr = blocking_handler
#pragma weak dma2_stream7_isr = blocking_handler
#pragma weak usart6_isr = blocking_handler
#pragma weak i2c3_ev_isr = blocking_handler
#pragma weak i2c3_er_isr = blocking_handler
#pragma weak otg_hs_ep1_out_isr = blocking_handler
#pragma weak otg_hs_ep1_in_isr = blocking_handler
#pragma weak otg_hs_wkup_isr = blocking_handler
#pragma weak otg_hs_isr = blocking_handler
#pragma weak dcmi_isr = blocking_handler
#pragma weak cryp_isr = blocking_handler
#pragma weak hash_rng_isr = blocking_handler
#pragma weak fpu_isr = blocking_handler
#pragma weak uart7_isr = blocking_handler
#pragma weak uart8_isr = blocking_handler
#pragma weak spi4_isr = blocking_handler
#pragma weak spi5_isr = blocking_handler
#pragma weak spi6_isr = blocking_handler
#pragma weak sai1_isr = blocking_handler
#pragma weak lcd_tft_isr = blocking_handler
#pragma weak lcd_tft_err_isr = blocking_handler
#pragma weak dma2d_isr = blocking_handler
#endif



void main(void);
void blocking_handler(void);
void null_handler(void);

typedef struct {
    unsigned int *initial_sp_value; /**< Initial stack pointer value. */
    vector_table_entry_t reset;
    vector_table_entry_t nmi;
    vector_table_entry_t hard_fault;
    vector_table_entry_t memory_manage_fault; /* not in CM0 */
    vector_table_entry_t bus_fault;           /* not in CM0 */
    vector_table_entry_t usage_fault;         /* not in CM0 */
    vector_table_entry_t reserved_x001c[4];
    vector_table_entry_t sv_call;
    vector_table_entry_t debug_monitor;       /* not in CM0 */
    vector_table_entry_t reserved_x0034;
    vector_table_entry_t pend_sv;
    vector_table_entry_t systick;
    vector_table_entry_t irq[NVIC_IRQ_COUNT];
} vector_table_t;

__attribute__ ((section(".vectors")))
vector_table_t vector_table = {
    .initial_sp_value = &_stack,
    .reset = reset_handler,
    .nmi = nmi_handler,
    .hard_fault = hard_fault_handler,

/* Those are defined only on CM3 or CM4 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    .memory_manage_fault = mem_manage_handler,
    .bus_fault = bus_fault_handler,
    .usage_fault = usage_fault_handler,
    .debug_monitor = debug_monitor_handler,
#endif

    .sv_call = sv_call_handler,
    .pend_sv = pend_sv_handler,
    .systick = sys_tick_handler,
    .irq = {
        IRQ_HANDLERS
    }
};

void WEAK __attribute__ ((naked)) reset_handler(void)
{
    volatile unsigned *src, *dest;
    funcp_t *fp;

    for (src = &_data_loadaddr, dest = &_data;
        dest < &_edata;
        src++, dest++) {
        *dest = *src;
    }

    while (dest < &_ebss) {
        *dest++ = 0;
    }

    /* Constructors. */
    for (fp = &__preinit_array_start; fp < &__preinit_array_end; fp++) {
        (*fp)();
    }
    for (fp = &__init_array_start; fp < &__init_array_end; fp++) {
        (*fp)();
    }

    /* Call the application's entry point. */
    main();

    /* Destructors. */
    for (fp = &__fini_array_start; fp < &__fini_array_end; fp++) {
        (*fp)();
    }

}

void blocking_handler(void)
{
    while (1);
}

void null_handler(void)
{
    /* Do nothing. */
}

/*
FOR f3/f4:
    static void pre_main(void)
{
    // Enable access to Floating-Point coprocessor. 
    SCB_CPACR |= SCB_CPACR_FULL * (SCB_CPACR_CP10 | SCB_CPACR_CP11);
}

*/