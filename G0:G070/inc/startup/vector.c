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

#define NVIC_WWDG_IRQ 0
#define NVIC_RTC_IRQ 2
#define NVIC_FLASH_IRQ 3
#define NVIC_RCC_IRQ 4
#define NVIC_EXTI0_1_IRQ 5
#define NVIC_EXTI2_3_IRQ 6
#define NVIC_EXTI4_15_IRQ 7
#define NVIC_DMA1_CHANNEL1_IRQ 9
#define NVIC_DMA1_CHANNEL2_3_IRQ 10
#define NVIC_DMAMUX_IRQ 11
#define NVIC_ADC_COMP_IRQ 12
#define NVIC_TIM1_BRK_UP_TRG_COM_IRQ 13
#define NVIC_TIM1_CC_IRQ 14
#define NVIC_TIM3_4_IRQ 16
#define NVIC_TIM6_DAC_IRQ 17
#define NVIC_TIM7_IRQ 18
#define NVIC_TIM14_IRQ 19
#define NVIC_TIM15_IRQ 20
#define NVIC_TIM16_IRQ 21
#define NVIC_TIM17_IRQ 22
#define NVIC_I2C1_IRQ 23
#define NVIC_I2C2_3_IRQ 24
#define NVIC_SPI1_IRQ 25
#define NVIC_SPI2_3_IRQ 26
#define NVIC_USART1_IRQ 27
#define NVIC_USART2_IRQ 28
#define NVIC_USART3_6_IRQ 29
#define NVIC_CEC_CAN_IRQ 30
#define NVIC_USB_IRQ 31

#define NVIC_IRQ_COUNT 32

#define IRQ_HANDLERS \
    [NVIC_WWDG_IRQ] = wwdg_isr, \
    [NVIC_RTC_IRQ] = rtc_isr, \
    [NVIC_FLASH_IRQ] = flash_isr, \
    [NVIC_RCC_IRQ] = rcc_isr, \
    [NVIC_EXTI0_1_IRQ] = exti0_1_isr, \
    [NVIC_EXTI2_3_IRQ] = exti2_3_isr, \
    [NVIC_EXTI4_15_IRQ] = exti4_15_isr, \
    [NVIC_DMA1_CHANNEL1_IRQ] = dma1_channel1_isr, \
    [NVIC_DMA1_CHANNEL2_3_IRQ] = dma1_channel2_3_isr, \
    [NVIC_DMAMUX_IRQ] = dmamux_isr, \
    [NVIC_ADC_COMP_IRQ] = adc_comp_isr, \
    [NVIC_TIM1_BRK_UP_TRG_COM_IRQ] = tim1_brk_up_trg_com_isr, \
    [NVIC_TIM1_CC_IRQ] = tim1_cc_isr, \
    [NVIC_TIM3_4_IRQ] = tim3_4_isr, \
    [NVIC_TIM6_DAC_IRQ] = tim6_dac_isr, \
    [NVIC_TIM7_IRQ] = tim7_isr, \
    [NVIC_TIM14_IRQ] = tim14_isr, \
    [NVIC_TIM15_IRQ] = tim15_isr, \
    [NVIC_TIM16_IRQ] = tim16_isr, \
    [NVIC_TIM17_IRQ] = tim17_isr, \
    [NVIC_I2C1_IRQ] = i2c1_isr, \
    [NVIC_I2C2_3_IRQ] = i2c2_3_isr, \
    [NVIC_SPI1_IRQ] = spi1_isr, \
    [NVIC_SPI2_3_IRQ] = spi2_3_isr, \
    [NVIC_USART1_IRQ] = usart1_isr, \
    [NVIC_USART2_IRQ] = usart2_isr, \
    [NVIC_USART3_6_IRQ] = usart3_6_isr, \
    [NVIC_CEC_CAN_IRQ] = cec_can_isr, \
    [NVIC_USB_IRQ] = usb_isr
#else
    #error "Not supported STM32 family"
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

vector_table_t vector_table __attribute__ ((section(".vector_table"))) = {
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

void WEAK __attribute__ ((naked)) __attribute__ ((noreturn)) reset_handler(void){
  extern char _sdata;    // .data section start
  extern char _edata;    // .data section end
  extern char _sbss;     // .bss  section start
  extern char _ebss;     // .bss  section end
  extern char _ldata;    // .data load address

  char *dst = &_sdata;
  char *src = &_ldata;

  // enable 8-byte stack alignment to comply with AAPCS
  SCB->CCR |= 0x00000200;

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

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#pragma weak mem_manage_handler = blocking_handler
#pragma weak bus_fault_handler = blocking_handler
#pragma weak usage_fault_handler = blocking_handler
#pragma weak debug_monitor_handler = null_handler
#endif

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
#pragma weak usart3_6_isr = blocking_handler
#pragma weak cec_can_isr = blocking_handler
#pragma weak usb_isr = blocking_handler
#endif

