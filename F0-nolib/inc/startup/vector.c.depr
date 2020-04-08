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
#include "stm32f0xx.h"

/* Initialization template for the interrupt vector table. This definition is
 * used by the startup code generator (vector.c) to set the initial values for
 * the interrupt handling routines to the chip family specific _isr weak
 * symbols. */
#define NVIC_WWDG_IRQ 0
#define NVIC_PVD_IRQ 1
#define NVIC_RTC_IRQ 2
#define NVIC_FLASH_IRQ 3
#define NVIC_RCC_IRQ 4
#define NVIC_EXTI0_1_IRQ 5
#define NVIC_EXTI2_3_IRQ 6
#define NVIC_EXTI4_15_IRQ 7
#define NVIC_TSC_IRQ 8
#define NVIC_DMA1_CHANNEL1_IRQ 9
#define NVIC_DMA1_CHANNEL2_3_IRQ 10
#define NVIC_DMA1_CHANNEL4_5_IRQ 11
#define NVIC_ADC_COMP_IRQ 12
#define NVIC_TIM1_BRK_UP_TRG_COM_IRQ 13
#define NVIC_TIM1_CC_IRQ 14
#define NVIC_TIM2_IRQ 15
#define NVIC_TIM3_IRQ 16
#define NVIC_TIM6_DAC_IRQ 17
#define NVIC_TIM7_IRQ 18
#define NVIC_TIM14_IRQ 19
#define NVIC_TIM15_IRQ 20
#define NVIC_TIM16_IRQ 21
#define NVIC_TIM17_IRQ 22
#define NVIC_I2C1_IRQ 23
#define NVIC_I2C2_IRQ 24
#define NVIC_SPI1_IRQ 25
#define NVIC_SPI2_IRQ 26
#define NVIC_USART1_IRQ 27
#define NVIC_USART2_IRQ 28
#define NVIC_USART3_4_IRQ 29
#define NVIC_CEC_CAN_IRQ 30
#define NVIC_USB_IRQ 31
#define NVIC_IRQ_COUNT 32

#define F0_IRQ_HANDLERS \
    [NVIC_WWDG_IRQ] = wwdg_isr, \
    [NVIC_PVD_IRQ] = pvd_isr, \
    [NVIC_RTC_IRQ] = rtc_isr, \
    [NVIC_FLASH_IRQ] = flash_isr, \
    [NVIC_RCC_IRQ] = rcc_isr, \
    [NVIC_EXTI0_1_IRQ] = exti0_1_isr, \
    [NVIC_EXTI2_3_IRQ] = exti2_3_isr, \
    [NVIC_EXTI4_15_IRQ] = exti4_15_isr, \
    [NVIC_TSC_IRQ] = tsc_isr, \
    [NVIC_DMA1_CHANNEL1_IRQ] = dma1_channel1_isr, \
    [NVIC_DMA1_CHANNEL2_3_IRQ] = dma1_channel2_3_isr, \
    [NVIC_DMA1_CHANNEL4_5_IRQ] = dma1_channel4_5_isr, \
    [NVIC_ADC_COMP_IRQ] = adc_comp_isr, \
    [NVIC_TIM1_BRK_UP_TRG_COM_IRQ] = tim1_brk_up_trg_com_isr, \
    [NVIC_TIM1_CC_IRQ] = tim1_cc_isr, \
    [NVIC_TIM2_IRQ] = tim2_isr, \
    [NVIC_TIM3_IRQ] = tim3_isr, \
    [NVIC_TIM6_DAC_IRQ] = tim6_dac_isr, \
    [NVIC_TIM7_IRQ] = tim7_isr, \
    [NVIC_TIM14_IRQ] = tim14_isr, \
    [NVIC_TIM15_IRQ] = tim15_isr, \
    [NVIC_TIM16_IRQ] = tim16_isr, \
    [NVIC_TIM17_IRQ] = tim17_isr, \
    [NVIC_I2C1_IRQ] = i2c1_isr, \
    [NVIC_I2C2_IRQ] = i2c2_isr, \
    [NVIC_SPI1_IRQ] = spi1_isr, \
    [NVIC_SPI2_IRQ] = spi2_isr, \
    [NVIC_USART1_IRQ] = usart1_isr, \
    [NVIC_USART2_IRQ] = usart2_isr, \
    [NVIC_USART3_4_IRQ] = usart3_4_isr, \
    [NVIC_CEC_CAN_IRQ] = cec_can_isr, \
    [NVIC_USB_IRQ] = usb_isr

typedef void (*vector_table_entry_t)(void);
typedef void (*funcp_t) (void);

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

/* Symbols exported by the linker script(s): */
extern unsigned _data_loadaddr, _data, _edata, _ebss, _stack;
extern funcp_t __preinit_array_start, __preinit_array_end;
extern funcp_t __init_array_start, __init_array_end;
extern funcp_t __fini_array_start, __fini_array_end;

void main(void);
void blocking_handler(void);
void null_handler(void);

__attribute__ ((section(".vectors")))
vector_table_t vector_table = {
    .initial_sp_value = &_stack,
    .reset = reset_handler,
    .nmi = nmi_handler,
    .hard_fault = hard_fault_handler,
    .sv_call = sv_call_handler,
    .pend_sv = pend_sv_handler,
    .systick = sys_tick_handler,
    .irq = {
        F0_IRQ_HANDLERS
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
