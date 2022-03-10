/*
 *                                                                                                  geany_encoding=koi8-r
 * usb_defs.h
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 *
 */

#pragma once
#ifndef __USB_DEFS_H__
#define __USB_DEFS_H__

#include <stm32f0.h>

// max endpoints number
#define STM32ENDPOINTS          8

/**
 *                 Buffers size definition
 **/
// !!! when working with CAN bus change USB_BTABLE_SIZE to 768 !!!
#define USB_BTABLE_SIZE         1024
// for USB FS EP0 buffers are from 8 to 64 bytes long (64 for PL2303)
#define USB_EP0_BUFSZ           64
// USB transmit buffer size (larger than keyboard report to prevent need of ZLP!)
#define USB_TXBUFSZ             10

#define USB_BTABLE_BASE         0x40006000
#undef USB_BTABLE
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

typedef struct{
    __IO uint32_t EPnR[STM32ENDPOINTS];
    __IO uint32_t RESERVED[STM32ENDPOINTS];
    __IO uint32_t CNTR;
    __IO uint32_t ISTR;
    __IO uint32_t FNR;
    __IO uint32_t DADDR;
    __IO uint32_t BTABLE;
    __IO uint32_t LPMCSR;
    __IO uint32_t BCDR;
} USB_TypeDef;

typedef struct{
    __IO uint16_t USB_ADDR_TX;
    __IO uint16_t USB_COUNT_TX;
    __IO uint16_t USB_ADDR_RX;
    __IO uint16_t USB_COUNT_RX;
} USB_EPDATA_TypeDef;

typedef struct{
    __IO USB_EPDATA_TypeDef EP[STM32ENDPOINTS];
} USB_BtableDef;

#endif // __USB_DEFS_H__
