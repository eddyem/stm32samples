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

typedef void (*vector_table_entry_t)(void);
typedef void (*funcp_t) (void);

void main(void);
void blocking_handler(void);
void null_handler(void);

/* Initialization template for the interrupt vector table. This definition is
 * used by the startup code generator (vector.c) to set the initial values for
 * the interrupt handling routines to the chip family specific _isr weak
 * symbols. */
#if defined STM32G0
#include "stm32g0xx.h"
#else
    #error "Not supported STM32 family"
#endif

#define NVIC_IRQ_COUNT 32

#if defined STM32G070xx
#define IRQ_HANDLERS \
    [WWDG_IRQn] = wwdg_isr, \
    [RTC_TAMP_IRQn] = rtc_isr, \
    [FLASH_IRQn] = flash_isr, \
    [RCC_IRQn] = rcc_isr, \
    [EXTI0_1_IRQn] = exti0_1_isr, \
    [EXTI2_3_IRQn] = exti2_3_isr, \
    [EXTI4_15_IRQn] = exti4_15_isr, \
    [DMA1_Channel1_IRQn] = dma1_channel1_isr, \
    [DMA1_Channel2_3_IRQn] = dma1_channel2_3_isr, \
    [DMA1_Ch4_7_DMAMUX1_OVR_IRQn] = dmamux_isr, \
    [ADC1_IRQn] = adc_comp_isr, \
    [TIM1_BRK_UP_TRG_COM_IRQn] = tim1_brk_up_trg_com_isr, \
    [TIM1_CC_IRQn] = tim1_cc_isr, \
    [TIM3_IRQn] = tim3_4_isr, \
    [TIM6_IRQn] = tim6_dac_isr, \
    [TIM7_IRQn] = tim7_isr, \
    [TIM14_IRQn] = tim14_isr, \
    [TIM15_IRQn] = tim15_isr, \
    [TIM16_IRQn] = tim16_isr, \
    [TIM17_IRQn] = tim17_isr, \
    [I2C1_IRQn] = i2c1_isr, \
    [I2C2_IRQn] = i2c2_3_isr, \
    [SPI1_IRQn] = spi1_isr, \
    [SPI2_IRQn] = spi2_3_isr, \
    [USART1_IRQn] = usart1_isr, \
    [USART2_IRQn] = usart2_isr, \
    [USART3_4_IRQn] = usart3_4_isr
#else
    #error "Not supported STM32G0 MCU"
#endif



typedef struct {
    unsigned int *initial_sp_value; /**< Initial stack pointer value. */
    vector_table_entry_t reset;
    vector_table_entry_t nmi;
    vector_table_entry_t hard_fault;
    vector_table_entry_t reserved1[7];
    vector_table_entry_t sv_call;
    vector_table_entry_t reserved2[2];
    vector_table_entry_t pend_sv;
    vector_table_entry_t systick;
    vector_table_entry_t irq[NVIC_IRQ_COUNT];
} vector_table_t;

extern unsigned _stack;

vector_table_t vector_table __attribute__ ((used, section(".vector_table"))) = {
    .initial_sp_value = &_stack,
    .reset = reset_handler,
    .nmi = nmi_handler,
    .hard_fault = hard_fault_handler,
    .sv_call = sv_call_handler,
    .pend_sv = pend_sv_handler,
    .systick = sys_tick_handler,
    .irq = {
        IRQ_HANDLERS
    }
};

void WEAK __attribute__ ((naked)) __attribute__ ((noreturn)) reset_handler(void){
  extern char _sdata;    // .data section start
  extern char _edata;    // .data section end
  extern char _sbss;     // .bss  section start
  extern char _ebss;     // .bss  section end
  extern char _ldata;    // .data load address

  char *dst = &_sdata;
  char *src = &_ldata;

  // copy initialized variables data
  while ( dst < &_edata ) { *dst++ = *src++; }

  // clear uninitialized variables
  for ( dst = &_sbss; dst < &_ebss; dst++ ) { *dst = 0; }

  // call main
  main();

  // halt
  for(;;) {}
}

void blocking_handler(void)
{
    while (1);
}

void null_handler(void)
{
    /* Do nothing. */
}

#pragma weak nmi_handler = null_handler
#pragma weak hard_fault_handler = blocking_handler
#pragma weak sv_call_handler = null_handler
#pragma weak pend_sv_handler = null_handler
#pragma weak sys_tick_handler = null_handler

#if defined STM32G0
#pragma weak wwdg_isr = blocking_handler
#pragma weak rtc_isr = blocking_handler
#pragma weak flash_isr = blocking_handler
#pragma weak rcc_isr = blocking_handler
#pragma weak exti0_1_isr = blocking_handler
#pragma weak exti2_3_isr = blocking_handler
#pragma weak exti4_15_isr = blocking_handler
#pragma weak dma1_channel1_isr = blocking_handler
#pragma weak dma1_channel2_3_isr = blocking_handler
#pragma weak dmamux_isr = blocking_handler
#pragma weak adc_comp_isr = blocking_handler
#pragma weak tim1_brk_up_trg_com_isr = blocking_handler
#pragma weak tim1_cc_isr = blocking_handler
#pragma weak tim3_4_isr = blocking_handler
#pragma weak tim6_dac_isr = blocking_handler
#pragma weak tim7_isr = blocking_handler
#pragma weak tim14_isr = blocking_handler
#pragma weak tim15_isr = blocking_handler
#pragma weak tim16_isr = blocking_handler
#pragma weak tim17_isr = blocking_handler
#pragma weak i2c1_isr = blocking_handler
#pragma weak i2c2_3_isr = blocking_handler
#pragma weak spi1_isr = blocking_handler
#pragma weak spi2_3_isr = blocking_handler
#pragma weak usart1_isr = blocking_handler
#pragma weak usart2_isr = blocking_handler
#pragma weak usart3_4_isr = blocking_handler
#pragma weak cec_can_isr = blocking_handler
#pragma weak usb_isr = blocking_handler
#endif

