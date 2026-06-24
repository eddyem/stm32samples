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
#include "common_macros.h"

// would change later
uint32_t SysFreq = 8000000;

typedef void (*vector_table_entry_t)(void);
typedef void (*funcp_t) (void);

void main(void);
void blocking_handler(void);
void null_handler(void);

#ifndef STM32G4
    #error "Not supported STM32 family"
#endif

#include "stm32g4xx.h"

#define NVIC_IRQ_COUNT 102

#if defined STM32G431xx
#define IRQ_HANDLERS \
    [WWDG_IRQn] = wwdg_isr, \
    [PVD_PVM_IRQn] = pvd_isr, \
    [RTC_TAMP_LSECSS_IRQn] = rtc_tamp_css_isr, \
    [RTC_WKUP_IRQn] = rtc_wkup_isr, \
    [FLASH_IRQn] = flash_isr, \
    [RCC_IRQn] = rcc_isr, \
    [EXTI0_IRQn] = exti0_isr, \
    [EXTI1_IRQn] = exti1_isr, \
    [EXTI2_IRQn] = exti2_isr, \
    [EXTI3_IRQn] = exti3_isr, \
    [EXTI4_IRQn] = exti4_isr, \
    [DMA1_Channel1_IRQn] = dma1_channel1_isr, \
    [DMA1_Channel2_IRQn] = dma1_channel2_isr, \
    [DMA1_Channel3_IRQn] = dma1_channel3_isr, \
    [DMA1_Channel4_IRQn] = dma1_channel4_isr, \
    [DMA1_Channel5_IRQn] = dma1_channel5_isr, \
    [DMA1_Channel6_IRQn] = dma1_channel6_isr, \
    [ADC1_2_IRQn] = adc12_isr, \
    [USB_HP_IRQn] = usb_hp_isr, \
    [USB_LP_IRQn] = usb_lp_isr, \
    [FDCAN1_IT0_IRQn] = fdcan1_it0_isr, \
    [FDCAN1_IT1_IRQn] = fdcan1_it1_isr, \
    [EXTI9_5_IRQn] = exti9_5_isr, \
    [TIM1_BRK_TIM15_IRQn] = tim1_brk_tim15_isr, \
    [TIM1_UP_TIM16_IRQn] = tim1_up_tim16_isr, \
    [TIM1_TRG_COM_TIM17_IRQn] = tim1_trg_tim17_isr, \
    [TIM1_CC_IRQn] = tim1_cc_isr, \
    [TIM2_IRQn] = tim2_isr, \
    [TIM3_IRQn] = tim3_isr, \
    [TIM4_IRQn] = tim4_isr, \
    [I2C1_EV_IRQn] = i2c1_ev_isr, \
    [I2C1_ER_IRQn] = i2c1_er_isr, \
    [I2C2_EV_IRQn] = i2c2_ev_isr, \
    [I2C2_ER_IRQn] = i2c2_er_isr, \
    [SPI1_IRQn] = spi1_isr, \
    [SPI2_IRQn] = spi2_isr, \
    [USART1_IRQn] = usart1_isr, \
    [USART2_IRQn] = usart2_isr, \
    [USART3_IRQn] = usart3_isr, \
    [EXTI15_10_IRQn] = exti15_10_isr, \
    [RTC_Alarm_IRQn] = rtc_alarm_isr, \
    [USBWakeUp_IRQn] = usb_wakeup_isr, \
    [TIM8_BRK_IRQn] = tim8_brk_isr, \
    [TIM8_UP_IRQn] = tim8_up_isr, \
    [TIM8_TRG_COM_IRQn] = tim8_trg_isr, \
    [TIM8_CC_IRQn] = tim8_cc_isr, \
    [LPTIM1_IRQn] = lptim1_isr, \
    [SPI3_IRQn] = spi3_isr, \
    [UART4_IRQn] = uart4_isr, \
    [TIM6_DAC_IRQn] = tim6_dac13under_isr, \
    [TIM7_IRQn] = tim7_dac24under_isr, \
    [DMA2_Channel1_IRQn] = dma2_channel1_isr, \
    [DMA2_Channel2_IRQn] = dma2_channel2_isr, \
    [DMA2_Channel3_IRQn] = dma2_channel3_isr, \
    [DMA2_Channel4_IRQn] = dma2_channel4_isr, \
    [DMA2_Channel5_IRQn] = dma2_channel5_isr, \
    [UCPD1_IRQn] = ucpd1_isr, \
    [COMP1_2_3_IRQn] = comp123_isr, \
    [COMP4_IRQn] = comp456_isr, \
    [CRS_IRQn] = crs_isr, \
    [SAI1_IRQn] = sai_isr, \
    [FPU_IRQn] = fpu_isr, \
    [RNG_IRQn] = rng_isr, \
    [LPUART1_IRQn] = lpuart_isr, \
    [I2C3_EV_IRQn] = i2c3_ev_isr, \
    [I2C3_ER_IRQn] = i2c3_er_isr, \
    [DMAMUX_OVR_IRQn] = dmamux_ovr_isr, \
    [DMA2_Channel6_IRQn] = dma2_channel6_isr, \
    [CORDIC_IRQn] = cordic_isr, \
    [FMAC_IRQn] = fmac_isr
#else
    #error "Not supported STM32G4 MCU"
#endif

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

extern unsigned _stack;

void WEAK __attribute__ ((naked)) __attribute__ ((noreturn)) reset_handler(void){
  extern uint32_t _sdata;    // .data section start
  extern uint32_t _edata;    // .data section end
  extern uint32_t _sbss;     // .bss  section start
  extern uint32_t _ebss;     // .bss  section end
  extern uint32_t _ldata;    // .data load address

  uint32_t *dst = &_sdata;
  uint32_t *src = &_ldata;

  // copy initialized variables data
  while ( dst < &_edata ) { *dst++ = *src++; }

  // clear uninitialized variables
  for ( dst = &_sbss; dst < &_ebss; dst++ ) { *dst = 0; }

  /* Enable access to Floating-Point coprocessor. */
#if (__FPU_PRESENT == 1)
    SCB->CPACR = 0x0f << 20 ;  /* set CP10 and CP11 Full Access */
    nop();
    __DSB();
    __ISB();
#endif

  // call main
  main();

  // halt
  for(;;) {}
}

void blocking_handler(void){
    while (1);
}

void null_handler(void){
    /* Do nothing. */
}

void WEAK nmi_handler(void);
void WEAK hard_fault_handler(void);
void WEAK sv_call_handler(void);
void WEAK pend_sv_handler(void);
void WEAK sys_tick_handler(void);
void WEAK mem_manage_handler(void);
void WEAK bus_fault_handler(void);
void WEAK usage_fault_handler(void);
void WEAK debug_monitor_handler(void);

#if defined STM32G431xx
void wwdg_isr(void) __attribute__((weak, alias("blocking_handler")));
void pvd_isr(void) __attribute__((weak, alias("blocking_handler")));
void rtc_tamp_css_isr(void) __attribute__((weak, alias("blocking_handler")));
void rtc_wkup_isr(void) __attribute__((weak, alias("blocking_handler")));
void flash_isr(void) __attribute__((weak, alias("blocking_handler")));
void rcc_isr(void) __attribute__((weak, alias("blocking_handler")));
void exti0_isr(void) __attribute__((weak, alias("blocking_handler")));
void exti1_isr(void) __attribute__((weak, alias("blocking_handler")));
void exti2_isr(void) __attribute__((weak, alias("blocking_handler")));
void exti3_isr(void) __attribute__((weak, alias("blocking_handler")));
void exti4_isr(void) __attribute__((weak, alias("blocking_handler")));
void dma1_channel1_isr(void) __attribute__((weak, alias("blocking_handler")));
void dma1_channel2_isr(void) __attribute__((weak, alias("blocking_handler")));
void dma1_channel3_isr(void) __attribute__((weak, alias("blocking_handler")));
void dma1_channel4_isr(void) __attribute__((weak, alias("blocking_handler")));
void dma1_channel5_isr(void) __attribute__((weak, alias("blocking_handler")));
void dma1_channel6_isr(void) __attribute__((weak, alias("blocking_handler")));
void dma1_channel7_isr(void) __attribute__((weak, alias("blocking_handler")));
void adc12_isr(void) __attribute__((weak, alias("blocking_handler")));
void usb_hp_isr(void) __attribute__((weak, alias("blocking_handler")));
void usb_lp_isr(void) __attribute__((weak, alias("blocking_handler")));
void fdcan1_it0_isr(void) __attribute__((weak, alias("blocking_handler")));
void fdcan1_it1_isr(void) __attribute__((weak, alias("blocking_handler")));
void exti9_5_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim1_brk_tim15_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim1_up_tim16_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim1_trg_tim17_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim1_cc_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim2_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim3_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim4_isr(void) __attribute__((weak, alias("blocking_handler")));
void i2c1_ev_isr(void) __attribute__((weak, alias("blocking_handler")));
void i2c1_er_isr(void) __attribute__((weak, alias("blocking_handler")));
void i2c2_ev_isr(void) __attribute__((weak, alias("blocking_handler")));
void i2c2_er_isr(void) __attribute__((weak, alias("blocking_handler")));
void spi1_isr(void) __attribute__((weak, alias("blocking_handler")));
void spi2_isr(void) __attribute__((weak, alias("blocking_handler")));
void usart1_isr(void) __attribute__((weak, alias("blocking_handler")));
void usart2_isr(void) __attribute__((weak, alias("blocking_handler")));
void usart3_isr(void) __attribute__((weak, alias("blocking_handler")));
void exti15_10_isr(void) __attribute__((weak, alias("blocking_handler")));
void rtc_alarm_isr(void) __attribute__((weak, alias("blocking_handler")));
void usb_wakeup_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim8_brk_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim8_up_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim8_trg_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim8_cc_isr(void) __attribute__((weak, alias("blocking_handler")));
void adc3_isr(void) __attribute__((weak, alias("blocking_handler")));
void fsmc_isr(void) __attribute__((weak, alias("blocking_handler")));
void lptim1_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim5_isr(void) __attribute__((weak, alias("blocking_handler")));
void spi3_isr(void) __attribute__((weak, alias("blocking_handler")));
void uart4_isr(void) __attribute__((weak, alias("blocking_handler")));
void uart5_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim6_dac13under_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim7_dac24under_isr(void) __attribute__((weak, alias("blocking_handler")));
void dma2_channel1_isr(void) __attribute__((weak, alias("blocking_handler")));
void dma2_channel2_isr(void) __attribute__((weak, alias("blocking_handler")));
void dma2_channel3_isr(void) __attribute__((weak, alias("blocking_handler")));
void dma2_channel4_isr(void) __attribute__((weak, alias("blocking_handler")));
void dma2_channel5_isr(void) __attribute__((weak, alias("blocking_handler")));
void adc4_isr(void) __attribute__((weak, alias("blocking_handler")));
void adc5_isr(void) __attribute__((weak, alias("blocking_handler")));
void ucpd1_isr(void) __attribute__((weak, alias("blocking_handler")));
void comp123_isr(void) __attribute__((weak, alias("blocking_handler")));
void comp456_isr(void) __attribute__((weak, alias("blocking_handler")));
void comp7_isr(void) __attribute__((weak, alias("blocking_handler")));
void hrtim_master_isr(void) __attribute__((weak, alias("blocking_handler")));
void hrtim_tima_isr(void) __attribute__((weak, alias("blocking_handler")));
void hrtim_timb_isr(void) __attribute__((weak, alias("blocking_handler")));
void hrtim_timc_isr(void) __attribute__((weak, alias("blocking_handler")));
void hrtim_timd_isr(void) __attribute__((weak, alias("blocking_handler")));
void hrtim_time_isr(void) __attribute__((weak, alias("blocking_handler")));
void hrtim_fault_isr(void) __attribute__((weak, alias("blocking_handler")));
void hrtim_timf_isr(void) __attribute__((weak, alias("blocking_handler")));
void crs_isr(void) __attribute__((weak, alias("blocking_handler")));
void sai_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim20_brk_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim20_up_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim20_trg_isr(void) __attribute__((weak, alias("blocking_handler")));
void tim20_cc_isr(void) __attribute__((weak, alias("blocking_handler")));
void fpu_isr(void) __attribute__((weak, alias("blocking_handler")));
void i2c4_ev_isr(void) __attribute__((weak, alias("blocking_handler")));
void i2c4_er_isr(void) __attribute__((weak, alias("blocking_handler")));
void spi4_isr(void) __attribute__((weak, alias("blocking_handler")));
void aes_isr(void) __attribute__((weak, alias("blocking_handler")));
void fdcan2_it0_isr(void) __attribute__((weak, alias("blocking_handler")));
void fdcan2_it1_isr(void) __attribute__((weak, alias("blocking_handler")));
void fdcan3_it0_isr(void) __attribute__((weak, alias("blocking_handler")));
void fdcan3_it1_isr(void) __attribute__((weak, alias("blocking_handler")));
void rng_isr(void) __attribute__((weak, alias("blocking_handler")));
void lpuart_isr(void) __attribute__((weak, alias("blocking_handler")));
void i2c3_ev_isr(void) __attribute__((weak, alias("blocking_handler")));
void i2c3_er_isr(void) __attribute__((weak, alias("blocking_handler")));
void dmamux_ovr_isr(void) __attribute__((weak, alias("blocking_handler")));
void quadspi_isr(void) __attribute__((weak, alias("blocking_handler")));
void dma1_channel8_isr(void) __attribute__((weak, alias("blocking_handler")));
void dma2_channel6_isr(void) __attribute__((weak, alias("blocking_handler")));
void dma2_channel7_isr(void) __attribute__((weak, alias("blocking_handler")));
void dma2_channel8_isr(void) __attribute__((weak, alias("blocking_handler")));
void cordic_isr(void) __attribute__((weak, alias("blocking_handler")));
void fmac_isr(void) __attribute__((weak, alias("blocking_handler")));
#endif


vector_table_t vector_table __attribute__ ((used, section(".vector_table"))) = {
    .initial_sp_value = &_stack,
    .reset = reset_handler,
    .nmi = nmi_handler,
    .hard_fault = hard_fault_handler,
    .memory_manage_fault = mem_manage_handler,
    .bus_fault = bus_fault_handler,
    .usage_fault = usage_fault_handler,
    .debug_monitor = debug_monitor_handler,
    .sv_call = sv_call_handler,
    .pend_sv = pend_sv_handler,
    .systick = sys_tick_handler,
    .irq = {
        IRQ_HANDLERS
    }
};
