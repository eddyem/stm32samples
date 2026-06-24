/*
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

#pragma once

#ifndef WEAK
#define WEAK __attribute__((weak))
#endif

#include <stdint.h>
extern uint32_t SysFreq;

void WEAK reset_handler(void);
void WEAK nmi_handler(void);
void WEAK hard_fault_handler(void);
void WEAK sv_call_handler(void);
void WEAK pend_sv_handler(void);
void WEAK sys_tick_handler(void);

#ifndef STM32G4
#error "Not supported platform, should be STM32G4!"
#endif

void WEAK wwdg_isr();
void WEAK pvd_isr();
void WEAK rtc_tamp_css_isr();
void WEAK rtc_wkup_isr();
void WEAK flash_isr();
void WEAK rcc_isr();
void WEAK exti0_isr();
void WEAK exti1_isr();
void WEAK exti2_isr();
void WEAK exti3_isr();
void WEAK exti4_isr();
void WEAK dma1_channel1_isr();
void WEAK dma1_channel2_isr();
void WEAK dma1_channel3_isr();
void WEAK dma1_channel4_isr();
void WEAK dma1_channel5_isr();
void WEAK dma1_channel6_isr();
void WEAK dma1_channel7_isr();
void WEAK adc12_isr();
void WEAK usb_hp_isr();
void WEAK usb_lp_isr();
void WEAK fdcan1_it0_isr();
void WEAK fdcan1_it1_isr();
void WEAK exti9_5_isr();
void WEAK tim1_brk_tim15_isr();
void WEAK tim1_up_tim16_isr();
void WEAK tim1_trg_tim17_isr();
void WEAK tim1_cc_isr();
void WEAK tim2_isr();
void WEAK tim3_isr();
void WEAK tim4_isr();
void WEAK i2c1_ev_isr();
void WEAK i2c1_er_isr();
void WEAK i2c2_ev_isr();
void WEAK i2c2_er_isr();
void WEAK spi1_isr();
void WEAK spi2_isr();
void WEAK usart1_isr();
void WEAK usart2_isr();
void WEAK usart3_isr();
void WEAK exti15_10_isr();
void WEAK rtc_alarm_isr();
void WEAK usb_wakeup_isr();
void WEAK tim8_brk_isr();
void WEAK tim8_up_isr();
void WEAK tim8_trg_isr();
void WEAK tim8_cc_isr();
void WEAK adc3_isr();
void WEAK fsmc_isr();
void WEAK lptim1_isr();
void WEAK tim5_isr();
void WEAK spi3_isr();
void WEAK uart4_isr();
void WEAK uart5_isr();
void WEAK tim6_dac13under_isr();
void WEAK tim7_dac24under_isr();
void WEAK dma2_channel1_isr();
void WEAK dma2_channel2_isr();
void WEAK dma2_channel3_isr();
void WEAK dma2_channel4_isr();
void WEAK dma2_channel5_isr();
void WEAK adc4_isr();
void WEAK adc5_isr();
void WEAK ucpd1_isr();
void WEAK comp123_isr();
void WEAK comp456_isr();
void WEAK comp7_isr();
void WEAK hrtim_master_isr();
void WEAK hrtim_tima_isr();
void WEAK hrtim_timb_isr();
void WEAK hrtim_timc_isr();
void WEAK hrtim_timd_isr();
void WEAK hrtim_time_isr();
void WEAK hrtim_fault_isr();
void WEAK hrtim_timf_isr();
void WEAK crs_isr();
void WEAK sai_isr();
void WEAK tim20_brk_isr();
void WEAK tim20_up_isr();
void WEAK tim20_trg_isr();
void WEAK tim20_cc_isr();
void WEAK fpu_isr();
void WEAK i2c4_ev_isr();
void WEAK i2c4_er_isr();
void WEAK spi4_isr();
void WEAK aes_isr();
void WEAK fdcan2_it0_isr();
void WEAK fdcan2_it1_isr();
void WEAK fdcan3_it0_isr();
void WEAK fdcan3_it1_isr();
void WEAK rng_isr();
void WEAK lpuart_isr();
void WEAK i2c3_ev_isr();
void WEAK i2c3_er_isr();
void WEAK dmamux_ovr_isr();
void WEAK quadspi_isr();
void WEAK dma1_channel8_isr();
void WEAK dma2_channel6_isr();
void WEAK dma2_channel7_isr();
void WEAK dma2_channel8_isr();
void WEAK cordic_isr();
void WEAK fmac_isr();


