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
#if defined STM32F0
#include "stm32f0xx.h"

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

#define IRQ_HANDLERS \
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

#elif defined STM32F1
#include "stm32f10x.h"

#define NVIC_WWDG_IRQ 0
#define NVIC_PVD_IRQ 1
#define NVIC_TAMPER_IRQ 2
#define NVIC_RTC_IRQ 3
#define NVIC_FLASH_IRQ 4
#define NVIC_RCC_IRQ 5
#define NVIC_EXTI0_IRQ 6
#define NVIC_EXTI1_IRQ 7
#define NVIC_EXTI2_IRQ 8
#define NVIC_EXTI3_IRQ 9
#define NVIC_EXTI4_IRQ 10
#define NVIC_DMA1_CHANNEL1_IRQ 11
#define NVIC_DMA1_CHANNEL2_IRQ 12
#define NVIC_DMA1_CHANNEL3_IRQ 13
#define NVIC_DMA1_CHANNEL4_IRQ 14
#define NVIC_DMA1_CHANNEL5_IRQ 15
#define NVIC_DMA1_CHANNEL6_IRQ 16
#define NVIC_DMA1_CHANNEL7_IRQ 17
#define NVIC_ADC1_2_IRQ 18
#define NVIC_USB_HP_CAN_TX_IRQ 19
#define NVIC_USB_LP_CAN_RX0_IRQ 20
#define NVIC_CAN_RX1_IRQ 21
#define NVIC_CAN_SCE_IRQ 22
#define NVIC_EXTI9_5_IRQ 23
#define NVIC_TIM1_BRK_IRQ 24
#define NVIC_TIM1_UP_IRQ 25
#define NVIC_TIM1_TRG_COM_IRQ 26
#define NVIC_TIM1_CC_IRQ 27
#define NVIC_TIM2_IRQ 28
#define NVIC_TIM3_IRQ 29
#define NVIC_TIM4_IRQ 30
#define NVIC_I2C1_EV_IRQ 31
#define NVIC_I2C1_ER_IRQ 32
#define NVIC_I2C2_EV_IRQ 33
#define NVIC_I2C2_ER_IRQ 34
#define NVIC_SPI1_IRQ 35
#define NVIC_SPI2_IRQ 36
#define NVIC_USART1_IRQ 37
#define NVIC_USART2_IRQ 38
#define NVIC_USART3_IRQ 39
#define NVIC_EXTI15_10_IRQ 40
#define NVIC_RTC_ALARM_IRQ 41
#define NVIC_USB_WAKEUP_IRQ 42
#define NVIC_TIM8_BRK_IRQ 43
#define NVIC_TIM8_UP_IRQ 44
#define NVIC_TIM8_TRG_COM_IRQ 45
#define NVIC_TIM8_CC_IRQ 46
#define NVIC_ADC3_IRQ 47
#define NVIC_FSMC_IRQ 48
#define NVIC_SDIO_IRQ 49
#define NVIC_TIM5_IRQ 50
#define NVIC_SPI3_IRQ 51
#define NVIC_UART4_IRQ 52
#define NVIC_UART5_IRQ 53
#define NVIC_TIM6_IRQ 54
#define NVIC_TIM7_IRQ 55
#define NVIC_DMA2_CHANNEL1_IRQ 56
#define NVIC_DMA2_CHANNEL2_IRQ 57
#define NVIC_DMA2_CHANNEL3_IRQ 58
#define NVIC_DMA2_CHANNEL4_5_IRQ 59
#define NVIC_DMA2_CHANNEL5_IRQ 60
#define NVIC_ETH_IRQ 61
#define NVIC_ETH_WKUP_IRQ 62
#define NVIC_CAN2_TX_IRQ 63
#define NVIC_CAN2_RX0_IRQ 64
#define NVIC_CAN2_RX1_IRQ 65
#define NVIC_CAN2_SCE_IRQ 66
#define NVIC_OTG_FS_IRQ 67

#define NVIC_IRQ_COUNT 68

#define IRQ_HANDLERS \
    [NVIC_WWDG_IRQ] = wwdg_isr, \
    [NVIC_PVD_IRQ] = pvd_isr, \
    [NVIC_TAMPER_IRQ] = tamper_isr, \
    [NVIC_RTC_IRQ] = rtc_isr, \
    [NVIC_FLASH_IRQ] = flash_isr, \
    [NVIC_RCC_IRQ] = rcc_isr, \
    [NVIC_EXTI0_IRQ] = exti0_isr, \
    [NVIC_EXTI1_IRQ] = exti1_isr, \
    [NVIC_EXTI2_IRQ] = exti2_isr, \
    [NVIC_EXTI3_IRQ] = exti3_isr, \
    [NVIC_EXTI4_IRQ] = exti4_isr, \
    [NVIC_DMA1_CHANNEL1_IRQ] = dma1_channel1_isr, \
    [NVIC_DMA1_CHANNEL2_IRQ] = dma1_channel2_isr, \
    [NVIC_DMA1_CHANNEL3_IRQ] = dma1_channel3_isr, \
    [NVIC_DMA1_CHANNEL4_IRQ] = dma1_channel4_isr, \
    [NVIC_DMA1_CHANNEL5_IRQ] = dma1_channel5_isr, \
    [NVIC_DMA1_CHANNEL6_IRQ] = dma1_channel6_isr, \
    [NVIC_DMA1_CHANNEL7_IRQ] = dma1_channel7_isr, \
    [NVIC_ADC1_2_IRQ] = adc1_2_isr, \
    [NVIC_USB_HP_CAN_TX_IRQ] = usb_hp_can_tx_isr, \
    [NVIC_USB_LP_CAN_RX0_IRQ] = usb_lp_can_rx0_isr, \
    [NVIC_CAN_RX1_IRQ] = can_rx1_isr, \
    [NVIC_CAN_SCE_IRQ] = can_sce_isr, \
    [NVIC_EXTI9_5_IRQ] = exti9_5_isr, \
    [NVIC_TIM1_BRK_IRQ] = tim1_brk_isr, \
    [NVIC_TIM1_UP_IRQ] = tim1_up_isr, \
    [NVIC_TIM1_TRG_COM_IRQ] = tim1_trg_com_isr, \
    [NVIC_TIM1_CC_IRQ] = tim1_cc_isr, \
    [NVIC_TIM2_IRQ] = tim2_isr, \
    [NVIC_TIM3_IRQ] = tim3_isr, \
    [NVIC_TIM4_IRQ] = tim4_isr, \
    [NVIC_I2C1_EV_IRQ] = i2c1_ev_isr, \
    [NVIC_I2C1_ER_IRQ] = i2c1_er_isr, \
    [NVIC_I2C2_EV_IRQ] = i2c2_ev_isr, \
    [NVIC_I2C2_ER_IRQ] = i2c2_er_isr, \
    [NVIC_SPI1_IRQ] = spi1_isr, \
    [NVIC_SPI2_IRQ] = spi2_isr, \
    [NVIC_USART1_IRQ] = usart1_isr, \
    [NVIC_USART2_IRQ] = usart2_isr, \
    [NVIC_USART3_IRQ] = usart3_isr, \
    [NVIC_EXTI15_10_IRQ] = exti15_10_isr, \
    [NVIC_RTC_ALARM_IRQ] = rtc_alarm_isr, \
    [NVIC_USB_WAKEUP_IRQ] = usb_wakeup_isr, \
    [NVIC_TIM8_BRK_IRQ] = tim8_brk_isr, \
    [NVIC_TIM8_UP_IRQ] = tim8_up_isr, \
    [NVIC_TIM8_TRG_COM_IRQ] = tim8_trg_com_isr, \
    [NVIC_TIM8_CC_IRQ] = tim8_cc_isr, \
    [NVIC_ADC3_IRQ] = adc3_isr, \
    [NVIC_FSMC_IRQ] = fsmc_isr, \
    [NVIC_SDIO_IRQ] = sdio_isr, \
    [NVIC_TIM5_IRQ] = tim5_isr, \
    [NVIC_SPI3_IRQ] = spi3_isr, \
    [NVIC_UART4_IRQ] = uart4_isr, \
    [NVIC_UART5_IRQ] = uart5_isr, \
    [NVIC_TIM6_IRQ] = tim6_isr, \
    [NVIC_TIM7_IRQ] = tim7_isr, \
    [NVIC_DMA2_CHANNEL1_IRQ] = dma2_channel1_isr, \
    [NVIC_DMA2_CHANNEL2_IRQ] = dma2_channel2_isr, \
    [NVIC_DMA2_CHANNEL3_IRQ] = dma2_channel3_isr, \
    [NVIC_DMA2_CHANNEL4_5_IRQ] = dma2_channel4_5_isr, \
    [NVIC_DMA2_CHANNEL5_IRQ] = dma2_channel5_isr, \
    [NVIC_ETH_IRQ] = eth_isr, \
    [NVIC_ETH_WKUP_IRQ] = eth_wkup_isr, \
    [NVIC_CAN2_TX_IRQ] = can2_tx_isr, \
    [NVIC_CAN2_RX0_IRQ] = can2_rx0_isr, \
    [NVIC_CAN2_RX1_IRQ] = can2_rx1_isr, \
    [NVIC_CAN2_SCE_IRQ] = can2_sce_isr, \
    [NVIC_OTG_FS_IRQ] = otg_fs_isr

#elif defined STM32F2

#define NVIC_NVIC_WWDG_IRQ 0
#define NVIC_PVD_IRQ 1
#define NVIC_TAMP_STAMP_IRQ 2
#define NVIC_RTC_WKUP_IRQ 3
#define NVIC_FLASH_IRQ 4
#define NVIC_RCC_IRQ 5
#define NVIC_EXTI0_IRQ 6
#define NVIC_EXTI1_IRQ 7
#define NVIC_EXTI2_IRQ 8
#define NVIC_EXTI3_IRQ 9
#define NVIC_EXTI4_IRQ 10
#define NVIC_DMA1_STREAM0_IRQ 11
#define NVIC_DMA1_STREAM1_IRQ 12
#define NVIC_DMA1_STREAM2_IRQ 13
#define NVIC_DMA1_STREAM3_IRQ 14
#define NVIC_DMA1_STREAM4_IRQ 15
#define NVIC_DMA1_STREAM5_IRQ 16
#define NVIC_DMA1_STREAM6_IRQ 17
#define NVIC_ADC_IRQ 18
#define NVIC_CAN1_TX_IRQ 19
#define NVIC_CAN1_RX0_IRQ 20
#define NVIC_CAN1_RX1_IRQ 21
#define NVIC_CAN1_SCE_IRQ 22
#define NVIC_EXTI9_5_IRQ 23
#define NVIC_TIM1_BRK_TIM9_IRQ 24
#define NVIC_TIM1_UP_TIM10_IRQ 25
#define NVIC_TIM1_TRG_COM_TIM11_IRQ 26
#define NVIC_TIM1_CC_IRQ 27
#define NVIC_TIM2_IRQ 28
#define NVIC_TIM3_IRQ 29
#define NVIC_TIM4_IRQ 30
#define NVIC_I2C1_EV_IRQ 31
#define NVIC_I2C1_ER_IRQ 32
#define NVIC_I2C2_EV_IRQ 33
#define NVIC_I2C2_ER_IRQ 34
#define NVIC_SPI1_IRQ 35
#define NVIC_SPI2_IRQ 36
#define NVIC_USART1_IRQ 37
#define NVIC_USART2_IRQ 38
#define NVIC_USART3_IRQ 39
#define NVIC_EXTI15_10_IRQ 40
#define NVIC_RTC_ALARM_IRQ 41
#define NVIC_USB_FS_WKUP_IRQ 42
#define NVIC_TIM8_BRK_TIM12_IRQ 43
#define NVIC_TIM8_UP_TIM13_IRQ 44
#define NVIC_TIM8_TRG_COM_TIM14_IRQ 45
#define NVIC_TIM8_CC_IRQ 46
#define NVIC_DMA1_STREAM7_IRQ 47
#define NVIC_FSMC_IRQ 48
#define NVIC_SDIO_IRQ 49
#define NVIC_TIM5_IRQ 50
#define NVIC_SPI3_IRQ 51
#define NVIC_UART4_IRQ 52
#define NVIC_UART5_IRQ 53
#define NVIC_TIM6_DAC_IRQ 54
#define NVIC_TIM7_IRQ 55
#define NVIC_DMA2_STREAM0_IRQ 56
#define NVIC_DMA2_STREAM1_IRQ 57
#define NVIC_DMA2_STREAM2_IRQ 58
#define NVIC_DMA2_STREAM3_IRQ 59
#define NVIC_DMA2_STREAM4_IRQ 60
#define NVIC_ETH_IRQ 61
#define NVIC_ETH_WKUP_IRQ 62
#define NVIC_CAN2_TX_IRQ 63
#define NVIC_CAN2_RX0_IRQ 64
#define NVIC_CAN2_RX1_IRQ 65
#define NVIC_CAN2_SCE_IRQ 66
#define NVIC_OTG_FS_IRQ 67
#define NVIC_DMA2_STREAM5_IRQ 68
#define NVIC_DMA2_STREAM6_IRQ 69
#define NVIC_DMA2_STREAM7_IRQ 70
#define NVIC_USART6_IRQ 71
#define NVIC_I2C3_EV_IRQ 72
#define NVIC_I2C3_ER_IRQ 73
#define NVIC_OTG_HS_EP1_OUT_IRQ 74
#define NVIC_OTG_HS_EP1_IN_IRQ 75
#define NVIC_OTG_HS_WKUP_IRQ 76
#define NVIC_OTG_HS_IRQ 77
#define NVIC_DCMI_IRQ 78
#define NVIC_CRYP_IRQ 79
#define NVIC_HASH_RNG_IRQ 80

#define NVIC_IRQ_COUNT 81
    
    #define IRQ_HANDLERS \
    [NVIC_NVIC_WWDG_IRQ] = nvic_wwdg_isr, \
    [NVIC_PVD_IRQ] = pvd_isr, \
    [NVIC_TAMP_STAMP_IRQ] = tamp_stamp_isr, \
    [NVIC_RTC_WKUP_IRQ] = rtc_wkup_isr, \
    [NVIC_FLASH_IRQ] = flash_isr, \
    [NVIC_RCC_IRQ] = rcc_isr, \
    [NVIC_EXTI0_IRQ] = exti0_isr, \
    [NVIC_EXTI1_IRQ] = exti1_isr, \
    [NVIC_EXTI2_IRQ] = exti2_isr, \
    [NVIC_EXTI3_IRQ] = exti3_isr, \
    [NVIC_EXTI4_IRQ] = exti4_isr, \
    [NVIC_DMA1_STREAM0_IRQ] = dma1_stream0_isr, \
    [NVIC_DMA1_STREAM1_IRQ] = dma1_stream1_isr, \
    [NVIC_DMA1_STREAM2_IRQ] = dma1_stream2_isr, \
    [NVIC_DMA1_STREAM3_IRQ] = dma1_stream3_isr, \
    [NVIC_DMA1_STREAM4_IRQ] = dma1_stream4_isr, \
    [NVIC_DMA1_STREAM5_IRQ] = dma1_stream5_isr, \
    [NVIC_DMA1_STREAM6_IRQ] = dma1_stream6_isr, \
    [NVIC_ADC_IRQ] = adc_isr, \
    [NVIC_CAN1_TX_IRQ] = can1_tx_isr, \
    [NVIC_CAN1_RX0_IRQ] = can1_rx0_isr, \
    [NVIC_CAN1_RX1_IRQ] = can1_rx1_isr, \
    [NVIC_CAN1_SCE_IRQ] = can1_sce_isr, \
    [NVIC_EXTI9_5_IRQ] = exti9_5_isr, \
    [NVIC_TIM1_BRK_TIM9_IRQ] = tim1_brk_tim9_isr, \
    [NVIC_TIM1_UP_TIM10_IRQ] = tim1_up_tim10_isr, \
    [NVIC_TIM1_TRG_COM_TIM11_IRQ] = tim1_trg_com_tim11_isr, \
    [NVIC_TIM1_CC_IRQ] = tim1_cc_isr, \
    [NVIC_TIM2_IRQ] = tim2_isr, \
    [NVIC_TIM3_IRQ] = tim3_isr, \
    [NVIC_TIM4_IRQ] = tim4_isr, \
    [NVIC_I2C1_EV_IRQ] = i2c1_ev_isr, \
    [NVIC_I2C1_ER_IRQ] = i2c1_er_isr, \
    [NVIC_I2C2_EV_IRQ] = i2c2_ev_isr, \
    [NVIC_I2C2_ER_IRQ] = i2c2_er_isr, \
    [NVIC_SPI1_IRQ] = spi1_isr, \
    [NVIC_SPI2_IRQ] = spi2_isr, \
    [NVIC_USART1_IRQ] = usart1_isr, \
    [NVIC_USART2_IRQ] = usart2_isr, \
    [NVIC_USART3_IRQ] = usart3_isr, \
    [NVIC_EXTI15_10_IRQ] = exti15_10_isr, \
    [NVIC_RTC_ALARM_IRQ] = rtc_alarm_isr, \
    [NVIC_USB_FS_WKUP_IRQ] = usb_fs_wkup_isr, \
    [NVIC_TIM8_BRK_TIM12_IRQ] = tim8_brk_tim12_isr, \
    [NVIC_TIM8_UP_TIM13_IRQ] = tim8_up_tim13_isr, \
    [NVIC_TIM8_TRG_COM_TIM14_IRQ] = tim8_trg_com_tim14_isr, \
    [NVIC_TIM8_CC_IRQ] = tim8_cc_isr, \
    [NVIC_DMA1_STREAM7_IRQ] = dma1_stream7_isr, \
    [NVIC_FSMC_IRQ] = fsmc_isr, \
    [NVIC_SDIO_IRQ] = sdio_isr, \
    [NVIC_TIM5_IRQ] = tim5_isr, \
    [NVIC_SPI3_IRQ] = spi3_isr, \
    [NVIC_UART4_IRQ] = uart4_isr, \
    [NVIC_UART5_IRQ] = uart5_isr, \
    [NVIC_TIM6_DAC_IRQ] = tim6_dac_isr, \
    [NVIC_TIM7_IRQ] = tim7_isr, \
    [NVIC_DMA2_STREAM0_IRQ] = dma2_stream0_isr, \
    [NVIC_DMA2_STREAM1_IRQ] = dma2_stream1_isr, \
    [NVIC_DMA2_STREAM2_IRQ] = dma2_stream2_isr, \
    [NVIC_DMA2_STREAM3_IRQ] = dma2_stream3_isr, \
    [NVIC_DMA2_STREAM4_IRQ] = dma2_stream4_isr, \
    [NVIC_ETH_IRQ] = eth_isr, \
    [NVIC_ETH_WKUP_IRQ] = eth_wkup_isr, \
    [NVIC_CAN2_TX_IRQ] = can2_tx_isr, \
    [NVIC_CAN2_RX0_IRQ] = can2_rx0_isr, \
    [NVIC_CAN2_RX1_IRQ] = can2_rx1_isr, \
    [NVIC_CAN2_SCE_IRQ] = can2_sce_isr, \
    [NVIC_OTG_FS_IRQ] = otg_fs_isr, \
    [NVIC_DMA2_STREAM5_IRQ] = dma2_stream5_isr, \
    [NVIC_DMA2_STREAM6_IRQ] = dma2_stream6_isr, \
    [NVIC_DMA2_STREAM7_IRQ] = dma2_stream7_isr, \
    [NVIC_USART6_IRQ] = usart6_isr, \
    [NVIC_I2C3_EV_IRQ] = i2c3_ev_isr, \
    [NVIC_I2C3_ER_IRQ] = i2c3_er_isr, \
    [NVIC_OTG_HS_EP1_OUT_IRQ] = otg_hs_ep1_out_isr, \
    [NVIC_OTG_HS_EP1_IN_IRQ] = otg_hs_ep1_in_isr, \
    [NVIC_OTG_HS_WKUP_IRQ] = otg_hs_wkup_isr, \
    [NVIC_OTG_HS_IRQ] = otg_hs_isr, \
    [NVIC_DCMI_IRQ] = dcmi_isr, \
    [NVIC_CRYP_IRQ] = cryp_isr, \
    [NVIC_HASH_RNG_IRQ] = hash_rng_isr

#elif defined STM32F3

#define NVIC_NVIC_WWDG_IRQ 0
#define NVIC_PVD_IRQ 1
#define NVIC_TAMP_STAMP_IRQ 2
#define NVIC_RTC_WKUP_IRQ 3
#define NVIC_FLASH_IRQ 4
#define NVIC_RCC_IRQ 5
#define NVIC_EXTI0_IRQ 6
#define NVIC_EXTI1_IRQ 7
#define NVIC_EXTI2_TSC_IRQ 8
#define NVIC_EXTI3_IRQ 9
#define NVIC_EXTI4_IRQ 10
#define NVIC_DMA1_CHANNEL1_IRQ 11
#define NVIC_DMA1_CHANNEL2_IRQ 12
#define NVIC_DMA1_CHANNEL3_IRQ 13
#define NVIC_DMA1_CHANNEL4_IRQ 14
#define NVIC_DMA1_CHANNEL5_IRQ 15
#define NVIC_DMA1_CHANNEL6_IRQ 16
#define NVIC_DMA1_CHANNEL7_IRQ 17
#define NVIC_ADC1_2_IRQ 18
#define NVIC_USB_HP_CAN1_TX_IRQ 19
#define NVIC_USB_LP_CAN1_RX0_IRQ 20
#define NVIC_CAN1_RX1_IRQ 21
#define NVIC_CAN1_SCE_IRQ 22
#define NVIC_EXTI9_5_IRQ 23
#define NVIC_TIM1_BRK_TIM15_IRQ 24
#define NVIC_TIM1_UP_TIM16_IRQ 25
#define NVIC_TIM1_TRG_COM_TIM17_IRQ 26
#define NVIC_TIM1_CC_IRQ 27
#define NVIC_TIM2_IRQ 28
#define NVIC_TIM3_IRQ 29
#define NVIC_TIM4_IRQ 30
#define NVIC_I2C1_EV_EXTI23_IRQ 31
#define NVIC_I2C1_ER_IRQ 32
#define NVIC_I2C2_EV_EXTI24_IRQ 33
#define NVIC_I2C2_ER_IRQ 34
#define NVIC_SPI1_IRQ 35
#define NVIC_SPI2_IRQ 36
#define NVIC_USART1_EXTI25_IRQ 37
#define NVIC_USART2_EXTI26_IRQ 38
#define NVIC_USART3_EXTI28_IRQ 39
#define NVIC_EXTI15_10_IRQ 40
#define NVIC_RTC_ALARM_IRQ 41
#define NVIC_USB_WKUP_A_IRQ 42
#define NVIC_TIM8_BRK_IRQ 43
#define NVIC_TIM8_UP_IRQ 44
#define NVIC_TIM8_TRG_COM_IRQ 45
#define NVIC_TIM8_CC_IRQ 46
#define NVIC_ADC3_IRQ 47
#define NVIC_RESERVED_1_IRQ 48
#define NVIC_RESERVED_2_IRQ 49
#define NVIC_RESERVED_3_IRQ 50
#define NVIC_SPI3_IRQ 51
#define NVIC_UART4_EXTI34_IRQ 52
#define NVIC_UART5_EXTI35_IRQ 53
#define NVIC_TIM6_DAC_IRQ 54
#define NVIC_TIM7_IRQ 55
#define NVIC_DMA2_CHANNEL1_IRQ 56
#define NVIC_DMA2_CHANNEL2_IRQ 57
#define NVIC_DMA2_CHANNEL3_IRQ 58
#define NVIC_DMA2_CHANNEL4_IRQ 59
#define NVIC_DMA2_CHANNEL5_IRQ 60
#define NVIC_ETH_IRQ 61
#define NVIC_RESERVED_4_IRQ 62
#define NVIC_RESERVED_5_IRQ 63
#define NVIC_COMP123_IRQ 64
#define NVIC_COMP456_IRQ 65
#define NVIC_COMP7_IRQ 66
#define NVIC_RESERVED_6_IRQ 67
#define NVIC_RESERVED_7_IRQ 68
#define NVIC_RESERVED_8_IRQ 69
#define NVIC_RESERVED_9_IRQ 70
#define NVIC_RESERVED_10_IRQ 71
#define NVIC_RESERVED_11_IRQ 72
#define NVIC_RESERVED_12_IRQ 73
#define NVIC_USB_HP_IRQ 74
#define NVIC_USB_LP_IRQ 75
#define NVIC_USB_WKUP_IRQ 76
#define NVIC_RESERVED_13_IRQ 77
#define NVIC_RESERVED_14_IRQ 78
#define NVIC_RESERVED_15_IRQ 79
#define NVIC_RESERVED_16_IRQ 80

#define NVIC_IRQ_COUNT 81
#define IRQ_HANDLERS \
    [NVIC_NVIC_WWDG_IRQ] = nvic_wwdg_isr, \
    [NVIC_PVD_IRQ] = pvd_isr, \
    [NVIC_TAMP_STAMP_IRQ] = tamp_stamp_isr, \
    [NVIC_RTC_WKUP_IRQ] = rtc_wkup_isr, \
    [NVIC_FLASH_IRQ] = flash_isr, \
    [NVIC_RCC_IRQ] = rcc_isr, \
    [NVIC_EXTI0_IRQ] = exti0_isr, \
    [NVIC_EXTI1_IRQ] = exti1_isr, \
    [NVIC_EXTI2_TSC_IRQ] = exti2_tsc_isr, \
    [NVIC_EXTI3_IRQ] = exti3_isr, \
    [NVIC_EXTI4_IRQ] = exti4_isr, \
    [NVIC_DMA1_CHANNEL1_IRQ] = dma1_channel1_isr, \
    [NVIC_DMA1_CHANNEL2_IRQ] = dma1_channel2_isr, \
    [NVIC_DMA1_CHANNEL3_IRQ] = dma1_channel3_isr, \
    [NVIC_DMA1_CHANNEL4_IRQ] = dma1_channel4_isr, \
    [NVIC_DMA1_CHANNEL5_IRQ] = dma1_channel5_isr, \
    [NVIC_DMA1_CHANNEL6_IRQ] = dma1_channel6_isr, \
    [NVIC_DMA1_CHANNEL7_IRQ] = dma1_channel7_isr, \
    [NVIC_ADC1_2_IRQ] = adc1_2_isr, \
    [NVIC_USB_HP_CAN1_TX_IRQ] = usb_hp_can1_tx_isr, \
    [NVIC_USB_LP_CAN1_RX0_IRQ] = usb_lp_can1_rx0_isr, \
    [NVIC_CAN1_RX1_IRQ] = can1_rx1_isr, \
    [NVIC_CAN1_SCE_IRQ] = can1_sce_isr, \
    [NVIC_EXTI9_5_IRQ] = exti9_5_isr, \
    [NVIC_TIM1_BRK_TIM15_IRQ] = tim1_brk_tim15_isr, \
    [NVIC_TIM1_UP_TIM16_IRQ] = tim1_up_tim16_isr, \
    [NVIC_TIM1_TRG_COM_TIM17_IRQ] = tim1_trg_com_tim17_isr, \
    [NVIC_TIM1_CC_IRQ] = tim1_cc_isr, \
    [NVIC_TIM2_IRQ] = tim2_isr, \
    [NVIC_TIM3_IRQ] = tim3_isr, \
    [NVIC_TIM4_IRQ] = tim4_isr, \
    [NVIC_I2C1_EV_EXTI23_IRQ] = i2c1_ev_exti23_isr, \
    [NVIC_I2C1_ER_IRQ] = i2c1_er_isr, \
    [NVIC_I2C2_EV_EXTI24_IRQ] = i2c2_ev_exti24_isr, \
    [NVIC_I2C2_ER_IRQ] = i2c2_er_isr, \
    [NVIC_SPI1_IRQ] = spi1_isr, \
    [NVIC_SPI2_IRQ] = spi2_isr, \
    [NVIC_USART1_EXTI25_IRQ] = usart1_exti25_isr, \
    [NVIC_USART2_EXTI26_IRQ] = usart2_exti26_isr, \
    [NVIC_USART3_EXTI28_IRQ] = usart3_exti28_isr, \
    [NVIC_EXTI15_10_IRQ] = exti15_10_isr, \
    [NVIC_RTC_ALARM_IRQ] = rtc_alarm_isr, \
    [NVIC_USB_WKUP_A_IRQ] = usb_wkup_a_isr, \
    [NVIC_TIM8_BRK_IRQ] = tim8_brk_isr, \
    [NVIC_TIM8_UP_IRQ] = tim8_up_isr, \
    [NVIC_TIM8_TRG_COM_IRQ] = tim8_trg_com_isr, \
    [NVIC_TIM8_CC_IRQ] = tim8_cc_isr, \
    [NVIC_ADC3_IRQ] = adc3_isr, \
    [NVIC_RESERVED_1_IRQ] = reserved_1_isr, \
    [NVIC_RESERVED_2_IRQ] = reserved_2_isr, \
    [NVIC_RESERVED_3_IRQ] = reserved_3_isr, \
    [NVIC_SPI3_IRQ] = spi3_isr, \
    [NVIC_UART4_EXTI34_IRQ] = uart4_exti34_isr, \
    [NVIC_UART5_EXTI35_IRQ] = uart5_exti35_isr, \
    [NVIC_TIM6_DAC_IRQ] = tim6_dac_isr, \
    [NVIC_TIM7_IRQ] = tim7_isr, \
    [NVIC_DMA2_CHANNEL1_IRQ] = dma2_channel1_isr, \
    [NVIC_DMA2_CHANNEL2_IRQ] = dma2_channel2_isr, \
    [NVIC_DMA2_CHANNEL3_IRQ] = dma2_channel3_isr, \
    [NVIC_DMA2_CHANNEL4_IRQ] = dma2_channel4_isr, \
    [NVIC_DMA2_CHANNEL5_IRQ] = dma2_channel5_isr, \
    [NVIC_ETH_IRQ] = eth_isr, \
    [NVIC_RESERVED_4_IRQ] = reserved_4_isr, \
    [NVIC_RESERVED_5_IRQ] = reserved_5_isr, \
    [NVIC_COMP123_IRQ] = comp123_isr, \
    [NVIC_COMP456_IRQ] = comp456_isr, \
    [NVIC_COMP7_IRQ] = comp7_isr, \
    [NVIC_RESERVED_6_IRQ] = reserved_6_isr, \
    [NVIC_RESERVED_7_IRQ] = reserved_7_isr, \
    [NVIC_RESERVED_8_IRQ] = reserved_8_isr, \
    [NVIC_RESERVED_9_IRQ] = reserved_9_isr, \
    [NVIC_RESERVED_10_IRQ] = reserved_10_isr, \
    [NVIC_RESERVED_11_IRQ] = reserved_11_isr, \
    [NVIC_RESERVED_12_IRQ] = reserved_12_isr, \
    [NVIC_USB_HP_IRQ] = usb_hp_isr, \
    [NVIC_USB_LP_IRQ] = usb_lp_isr, \
    [NVIC_USB_WKUP_IRQ] = usb_wkup_isr, \
    [NVIC_RESERVED_13_IRQ] = reserved_13_isr, \
    [NVIC_RESERVED_14_IRQ] = reserved_14_isr, \
    [NVIC_RESERVED_15_IRQ] = reserved_15_isr, \
    [NVIC_RESERVED_16_IRQ] = reserved_16_isr

#elif defined STM32F4

#define NVIC_NVIC_WWDG_IRQ 0
#define NVIC_PVD_IRQ 1
#define NVIC_TAMP_STAMP_IRQ 2
#define NVIC_RTC_WKUP_IRQ 3
#define NVIC_FLASH_IRQ 4
#define NVIC_RCC_IRQ 5
#define NVIC_EXTI0_IRQ 6
#define NVIC_EXTI1_IRQ 7
#define NVIC_EXTI2_IRQ 8
#define NVIC_EXTI3_IRQ 9
#define NVIC_EXTI4_IRQ 10
#define NVIC_DMA1_STREAM0_IRQ 11
#define NVIC_DMA1_STREAM1_IRQ 12
#define NVIC_DMA1_STREAM2_IRQ 13
#define NVIC_DMA1_STREAM3_IRQ 14
#define NVIC_DMA1_STREAM4_IRQ 15
#define NVIC_DMA1_STREAM5_IRQ 16
#define NVIC_DMA1_STREAM6_IRQ 17
#define NVIC_ADC_IRQ 18
#define NVIC_CAN1_TX_IRQ 19
#define NVIC_CAN1_RX0_IRQ 20
#define NVIC_CAN1_RX1_IRQ 21
#define NVIC_CAN1_SCE_IRQ 22
#define NVIC_EXTI9_5_IRQ 23
#define NVIC_TIM1_BRK_TIM9_IRQ 24
#define NVIC_TIM1_UP_TIM10_IRQ 25
#define NVIC_TIM1_TRG_COM_TIM11_IRQ 26
#define NVIC_TIM1_CC_IRQ 27
#define NVIC_TIM2_IRQ 28
#define NVIC_TIM3_IRQ 29
#define NVIC_TIM4_IRQ 30
#define NVIC_I2C1_EV_IRQ 31
#define NVIC_I2C1_ER_IRQ 32
#define NVIC_I2C2_EV_IRQ 33
#define NVIC_I2C2_ER_IRQ 34
#define NVIC_SPI1_IRQ 35
#define NVIC_SPI2_IRQ 36
#define NVIC_USART1_IRQ 37
#define NVIC_USART2_IRQ 38
#define NVIC_USART3_IRQ 39
#define NVIC_EXTI15_10_IRQ 40
#define NVIC_RTC_ALARM_IRQ 41
#define NVIC_USB_FS_WKUP_IRQ 42
#define NVIC_TIM8_BRK_TIM12_IRQ 43
#define NVIC_TIM8_UP_TIM13_IRQ 44
#define NVIC_TIM8_TRG_COM_TIM14_IRQ 45
#define NVIC_TIM8_CC_IRQ 46
#define NVIC_DMA1_STREAM7_IRQ 47
#define NVIC_FSMC_IRQ 48
#define NVIC_SDIO_IRQ 49
#define NVIC_TIM5_IRQ 50
#define NVIC_SPI3_IRQ 51
#define NVIC_UART4_IRQ 52
#define NVIC_UART5_IRQ 53
#define NVIC_TIM6_DAC_IRQ 54
#define NVIC_TIM7_IRQ 55
#define NVIC_DMA2_STREAM0_IRQ 56
#define NVIC_DMA2_STREAM1_IRQ 57
#define NVIC_DMA2_STREAM2_IRQ 58
#define NVIC_DMA2_STREAM3_IRQ 59
#define NVIC_DMA2_STREAM4_IRQ 60
#define NVIC_ETH_IRQ 61
#define NVIC_ETH_WKUP_IRQ 62
#define NVIC_CAN2_TX_IRQ 63
#define NVIC_CAN2_RX0_IRQ 64
#define NVIC_CAN2_RX1_IRQ 65
#define NVIC_CAN2_SCE_IRQ 66
#define NVIC_OTG_FS_IRQ 67
#define NVIC_DMA2_STREAM5_IRQ 68
#define NVIC_DMA2_STREAM6_IRQ 69
#define NVIC_DMA2_STREAM7_IRQ 70
#define NVIC_USART6_IRQ 71
#define NVIC_I2C3_EV_IRQ 72
#define NVIC_I2C3_ER_IRQ 73
#define NVIC_OTG_HS_EP1_OUT_IRQ 74
#define NVIC_OTG_HS_EP1_IN_IRQ 75
#define NVIC_OTG_HS_WKUP_IRQ 76
#define NVIC_OTG_HS_IRQ 77
#define NVIC_DCMI_IRQ 78
#define NVIC_CRYP_IRQ 79
#define NVIC_HASH_RNG_IRQ 80
#define NVIC_FPU_IRQ 81
#define NVIC_UART7_IRQ 82
#define NVIC_UART8_IRQ 83
#define NVIC_SPI4_IRQ 84
#define NVIC_SPI5_IRQ 85
#define NVIC_SPI6_IRQ 86
#define NVIC_SAI1_IRQ 87
#define NVIC_LCD_TFT_IRQ 88
#define NVIC_LCD_TFT_ERR_IRQ 89
#define NVIC_DMA2D_IRQ 90

#define NVIC_IRQ_COUNT 91
    #define IRQ_HANDLERS \
    [NVIC_NVIC_WWDG_IRQ] = nvic_wwdg_isr, \
    [NVIC_PVD_IRQ] = pvd_isr, \
    [NVIC_TAMP_STAMP_IRQ] = tamp_stamp_isr, \
    [NVIC_RTC_WKUP_IRQ] = rtc_wkup_isr, \
    [NVIC_FLASH_IRQ] = flash_isr, \
    [NVIC_RCC_IRQ] = rcc_isr, \
    [NVIC_EXTI0_IRQ] = exti0_isr, \
    [NVIC_EXTI1_IRQ] = exti1_isr, \
    [NVIC_EXTI2_IRQ] = exti2_isr, \
    [NVIC_EXTI3_IRQ] = exti3_isr, \
    [NVIC_EXTI4_IRQ] = exti4_isr, \
    [NVIC_DMA1_STREAM0_IRQ] = dma1_stream0_isr, \
    [NVIC_DMA1_STREAM1_IRQ] = dma1_stream1_isr, \
    [NVIC_DMA1_STREAM2_IRQ] = dma1_stream2_isr, \
    [NVIC_DMA1_STREAM3_IRQ] = dma1_stream3_isr, \
    [NVIC_DMA1_STREAM4_IRQ] = dma1_stream4_isr, \
    [NVIC_DMA1_STREAM5_IRQ] = dma1_stream5_isr, \
    [NVIC_DMA1_STREAM6_IRQ] = dma1_stream6_isr, \
    [NVIC_ADC_IRQ] = adc_isr, \
    [NVIC_CAN1_TX_IRQ] = can1_tx_isr, \
    [NVIC_CAN1_RX0_IRQ] = can1_rx0_isr, \
    [NVIC_CAN1_RX1_IRQ] = can1_rx1_isr, \
    [NVIC_CAN1_SCE_IRQ] = can1_sce_isr, \
    [NVIC_EXTI9_5_IRQ] = exti9_5_isr, \
    [NVIC_TIM1_BRK_TIM9_IRQ] = tim1_brk_tim9_isr, \
    [NVIC_TIM1_UP_TIM10_IRQ] = tim1_up_tim10_isr, \
    [NVIC_TIM1_TRG_COM_TIM11_IRQ] = tim1_trg_com_tim11_isr, \
    [NVIC_TIM1_CC_IRQ] = tim1_cc_isr, \
    [NVIC_TIM2_IRQ] = tim2_isr, \
    [NVIC_TIM3_IRQ] = tim3_isr, \
    [NVIC_TIM4_IRQ] = tim4_isr, \
    [NVIC_I2C1_EV_IRQ] = i2c1_ev_isr, \
    [NVIC_I2C1_ER_IRQ] = i2c1_er_isr, \
    [NVIC_I2C2_EV_IRQ] = i2c2_ev_isr, \
    [NVIC_I2C2_ER_IRQ] = i2c2_er_isr, \
    [NVIC_SPI1_IRQ] = spi1_isr, \
    [NVIC_SPI2_IRQ] = spi2_isr, \
    [NVIC_USART1_IRQ] = usart1_isr, \
    [NVIC_USART2_IRQ] = usart2_isr, \
    [NVIC_USART3_IRQ] = usart3_isr, \
    [NVIC_EXTI15_10_IRQ] = exti15_10_isr, \
    [NVIC_RTC_ALARM_IRQ] = rtc_alarm_isr, \
    [NVIC_USB_FS_WKUP_IRQ] = usb_fs_wkup_isr, \
    [NVIC_TIM8_BRK_TIM12_IRQ] = tim8_brk_tim12_isr, \
    [NVIC_TIM8_UP_TIM13_IRQ] = tim8_up_tim13_isr, \
    [NVIC_TIM8_TRG_COM_TIM14_IRQ] = tim8_trg_com_tim14_isr, \
    [NVIC_TIM8_CC_IRQ] = tim8_cc_isr, \
    [NVIC_DMA1_STREAM7_IRQ] = dma1_stream7_isr, \
    [NVIC_FSMC_IRQ] = fsmc_isr, \
    [NVIC_SDIO_IRQ] = sdio_isr, \
    [NVIC_TIM5_IRQ] = tim5_isr, \
    [NVIC_SPI3_IRQ] = spi3_isr, \
    [NVIC_UART4_IRQ] = uart4_isr, \
    [NVIC_UART5_IRQ] = uart5_isr, \
    [NVIC_TIM6_DAC_IRQ] = tim6_dac_isr, \
    [NVIC_TIM7_IRQ] = tim7_isr, \
    [NVIC_DMA2_STREAM0_IRQ] = dma2_stream0_isr, \
    [NVIC_DMA2_STREAM1_IRQ] = dma2_stream1_isr, \
    [NVIC_DMA2_STREAM2_IRQ] = dma2_stream2_isr, \
    [NVIC_DMA2_STREAM3_IRQ] = dma2_stream3_isr, \
    [NVIC_DMA2_STREAM4_IRQ] = dma2_stream4_isr, \
    [NVIC_ETH_IRQ] = eth_isr, \
    [NVIC_ETH_WKUP_IRQ] = eth_wkup_isr, \
    [NVIC_CAN2_TX_IRQ] = can2_tx_isr, \
    [NVIC_CAN2_RX0_IRQ] = can2_rx0_isr, \
    [NVIC_CAN2_RX1_IRQ] = can2_rx1_isr, \
    [NVIC_CAN2_SCE_IRQ] = can2_sce_isr, \
    [NVIC_OTG_FS_IRQ] = otg_fs_isr, \
    [NVIC_DMA2_STREAM5_IRQ] = dma2_stream5_isr, \
    [NVIC_DMA2_STREAM6_IRQ] = dma2_stream6_isr, \
    [NVIC_DMA2_STREAM7_IRQ] = dma2_stream7_isr, \
    [NVIC_USART6_IRQ] = usart6_isr, \
    [NVIC_I2C3_EV_IRQ] = i2c3_ev_isr, \
    [NVIC_I2C3_ER_IRQ] = i2c3_er_isr, \
    [NVIC_OTG_HS_EP1_OUT_IRQ] = otg_hs_ep1_out_isr, \
    [NVIC_OTG_HS_EP1_IN_IRQ] = otg_hs_ep1_in_isr, \
    [NVIC_OTG_HS_WKUP_IRQ] = otg_hs_wkup_isr, \
    [NVIC_OTG_HS_IRQ] = otg_hs_isr, \
    [NVIC_DCMI_IRQ] = dcmi_isr, \
    [NVIC_CRYP_IRQ] = cryp_isr, \
    [NVIC_HASH_RNG_IRQ] = hash_rng_isr, \
    [NVIC_FPU_IRQ] = fpu_isr, \
    [NVIC_UART7_IRQ] = uart7_isr, \
    [NVIC_UART8_IRQ] = uart8_isr, \
    [NVIC_SPI4_IRQ] = spi4_isr, \
    [NVIC_SPI5_IRQ] = spi5_isr, \
    [NVIC_SPI6_IRQ] = spi6_isr, \
    [NVIC_SAI1_IRQ] = sai1_isr, \
    [NVIC_LCD_TFT_IRQ] = lcd_tft_isr, \
    [NVIC_LCD_TFT_ERR_IRQ] = lcd_tft_err_isr, \
    [NVIC_DMA2D_IRQ] = dma2d_isr
#else 
    #error "Not supported STM32 family"
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

#if defined STM32F0
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


/*
FOR f3/f4:
    static void pre_main(void)
{
    // Enable access to Floating-Point coprocessor. 
    SCB_CPACR |= SCB_CPACR_FULL * (SCB_CPACR_CP10 | SCB_CPACR_CP11);
}

*/