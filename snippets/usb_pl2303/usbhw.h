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
#pragma once

#if defined STM32F0
#include <stm32f0.h>
#elif defined STM32F1
#include <stm32f1.h>
// there's no this define in standard header
#define USB_BASE                ((uint32_t)0x40005C00)
#elif defined STM32F3
#include <stm32f3.h>
#endif

// max endpoints number
#define STM32ENDPOINTS          8
/**
 *                 Buffers size definition
 **/

//  F0 - USB2_16; F1 - USB1_16; F3 - 1/2 depending on series
#if !defined USB1_16 && !defined USB2_16
#if defined STM32F0
#define USB2_16
#elif defined STM32F1
#define USB1_16
#else
#error "Can't determine USB1_16 or USB2_16, define by hands"
#endif
#endif

// BTABLE_SIZE FOR STM32F3:
// In STM32F303/302xB/C, 512 bytes SRAM is not shared with CAN.
// In STM32F302x6/x8 and STM32F30xxD/E, 726 bytes dedicated SRAM and 256 bytes shared SRAM with CAN i.e.
//     1Kbytes dedicated SRAM in case CAN is disabled.
// remember, that USB_BTABLE_SIZE will be divided by ACCESSZ, so don't divide it twice for 32-bit addressing

#ifdef NOCAN
#if defined STM32F0
#define USB_BTABLE_SIZE         1024
#elif defined STM32F3
#define USB_BTABLE_SIZE         512
#warning "Please, check real buffer size due to docs"
#else
#error "define STM32F0 or STM32F3"
#endif
#else // !NOCAN: F0/F3 with CAN or F1 (can't simultaneously run CAN and USB)
#if defined STM32F0
#define USB_BTABLE_SIZE         768
#elif defined STM32F3
#define USB_BTABLE_SIZE         512
#warning "Please, check real buffer size due to docs"
#else // STM32F103: 1024 bytes but with 32-bit addressing
#define USB_BTABLE_SIZE         1024
#endif
#endif // NOCAN

// first 64 bytes of USB_BTABLE are registers!
//#define USB_EP0_BASEADDR        64
// for USB FS EP0 buffers are from 8 to 64 bytes long (64 for PL2303)
#define USB_EP0_BUFSZ           64
// USB transmit buffer size (64 for PL2303)
#define USB_TXBUFSZ             64
// USB receive buffer size (64 for PL2303)
#define USB_RXBUFSZ             64
// EP1 - interrupt - buffer size
#define USB_EP1BUFSZ            8

#define USB_BTABLE_BASE         0x40006000
#define USB                     ((USB_TypeDef *) USB_BASE)

#ifdef USB_BTABLE
#undef USB_BTABLE
#endif
#define USB_BTABLE              ((USB_BtableDef *)(USB_BTABLE_BASE))
#define USB_ISTR_EPID           0x0000000F
#define USB_FNR_LSOF_0          0x00000800
#define USB_FNR_lSOF_1          0x00001000
#define USB_LPMCSR_BESL_0       0x00000010
#define USB_LPMCSR_BESL_1       0x00000020
#define USB_LPMCSR_BESL_2       0x00000040
#define USB_LPMCSR_BESL_3       0x00000080
#define USB_EPnR_CTR_RX         0x00008000
#define USB_EPnR_DTOG_RX        0x00004000
#define USB_EPnR_STAT_RX        0x00003000
#define USB_EPnR_STAT_RX_0      0x00001000
#define USB_EPnR_STAT_RX_1      0x00002000
#define USB_EPnR_SETUP          0x00000800
#define USB_EPnR_EP_TYPE        0x00000600
#define USB_EPnR_EP_TYPE_0      0x00000200
#define USB_EPnR_EP_TYPE_1      0x00000400
#define USB_EPnR_EP_KIND        0x00000100
#define USB_EPnR_CTR_TX         0x00000080
#define USB_EPnR_DTOG_TX        0x00000040
#define USB_EPnR_STAT_TX        0x00000030
#define USB_EPnR_STAT_TX_0      0x00000010
#define USB_EPnR_STAT_TX_1      0x00000020
#define USB_EPnR_EA             0x0000000F
#define USB_COUNTn_RX_BLSIZE    0x00008000
#define USB_COUNTn_NUM_BLOCK    0x00007C00
#define USB_COUNTn_RX           0x0000003F

#define USB_TypeDef USB_TypeDef_custom

typedef struct {
    __IO uint32_t EPnR[STM32ENDPOINTS];
    __IO uint32_t RESERVED[STM32ENDPOINTS];
    __IO uint32_t CNTR;
    __IO uint32_t ISTR;
    __IO uint32_t FNR;
    __IO uint32_t DADDR;
    __IO uint32_t BTABLE;
} USB_TypeDef;

// F303 D/E have 2x16 access scheme
typedef struct{
#if defined USB2_16
    __IO uint16_t USB_ADDR_TX;
    __IO uint16_t USB_COUNT_TX;
    __IO uint16_t USB_ADDR_RX;
    __IO uint16_t USB_COUNT_RX;
#define ACCESSZ (1)
#define BUFTYPE uint8_t
#elif defined USB1_16
    __IO uint32_t USB_ADDR_TX;
    __IO uint32_t USB_COUNT_TX;
    __IO uint32_t USB_ADDR_RX;
    __IO uint32_t USB_COUNT_RX;
#define ACCESSZ (2)
#define BUFTYPE uint16_t
#else
#error "Define USB1_16 or USB2_16"
#endif
} USB_EPDATA_TypeDef;


typedef struct{
    __IO USB_EPDATA_TypeDef EP[STM32ENDPOINTS];
} USB_BtableDef;

void USB_setup();
