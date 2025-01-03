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

#include <stdint.h>
#include <wchar.h>

/******************************************************************
 *             Hardware registers etc                             *
 *****************************************************************/
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
#ifdef STM32F0
    __IO uint32_t LPMCSR;
    __IO uint32_t BCDR;
#endif
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

#define EP0DATABUF_SIZE                 (64)
#define LASTADDR_DEFAULT                (STM32ENDPOINTS * 8)

/******************************************************************
 *             Defines from usb.h                                 *
 *****************************************************************/

/*
 * Device and/or Interface Class codes
 */
#define USB_CLASS_PER_INTERFACE         0
#define USB_CLASS_AUDIO                 1
#define USB_CLASS_COMM                  2
#define USB_CLASS_HID                   3
#define USB_CLASS_PRINTER               7
#define USB_CLASS_PTP                   6
#define USB_CLASS_MASS_STORAGE          8
#define USB_CLASS_HUB                   9
#define USB_CLASS_DATA                  10
#define USB_CLASS_MISC                  0xef
#define USB_CLASS_VENDOR_SPEC           0xff

/*
 * Descriptor types
 */
#define USB_DT_DEVICE                   0x01
#define USB_DT_CONFIG                   0x02
#define USB_DT_STRING                   0x03
#define USB_DT_INTERFACE                0x04
#define USB_DT_ENDPOINT                 0x05
#define USB_DT_QUALIFIER                0x06

#define USB_DT_HID                      0x21
#define USB_DT_REPORT                   0x22
#define USB_DT_PHYSICAL                 0x23
#define USB_DT_CS_INTERFACE             0x24
#define USB_DT_HUB                      0x29

/*
 * Descriptor sizes per descriptor type
 */
#define USB_DT_DEVICE_SIZE              18
#define USB_DT_CONFIG_SIZE              9
#define USB_DT_INTERFACE_SIZE           9
#define USB_DT_HID_SIZE                 9
#define USB_DT_ENDPOINT_SIZE            7
#define USB_DT_QUALIFIER_SIZE           10
#define USB_DT_CS_INTERFACE_SIZE        5



// bmRequestType & 0x80 == dev2host (1) or host2dev (0)
// recipient: bmRequestType & 0x1f
#define REQUEST_RECIPIENT(b)    (b & 0x1f)
#define REQ_RECIPIENT_DEVICE    0
#define REQ_RECIPIENT_INTERFACE 1
#define REQ_RECIPIENT_ENDPOINT  2
#define REQ_RECIPIENT_OTHER     3
// type: [bmRequestType & 0x60 >> 5]
#define REQUEST_TYPE(b)         ((b&0x60)>>5)
#define REQ_TYPE_STANDARD       0
#define REQ_TYPE_CLASS          1
#define REQ_TYPE_VENDOR         2
#define REQ_TYPE_RESERVED       3


//#define VENDOR_REQUEST                  0x01

// standard device requests
#define GET_STATUS                      0x00
#define CLEAR_FEATURE                   0x01
#define SET_FEATURE                     0x03
#define SET_ADDRESS                     0x05
#define GET_DESCRIPTOR                  0x06
#define SET_DESCRIPTOR                  0x07
#define GET_CONFIGURATION               0x08
#define SET_CONFIGURATION               0x09
// and some standard interface requests
#define GET_INTERFACE                   0x0A
#define SET_INTERFACE                   0x0B
// and some standard endpoint requests
#define SYNC_FRAME                      0x0C

// Types of descriptors
#define DEVICE_DESCRIPTOR               0x01
#define CONFIGURATION_DESCRIPTOR        0x02
#define STRING_DESCRIPTOR               0x03
#define DEVICE_QUALIFIER_DESCRIPTOR     0x06
#define DEBUG_DESCRIPTOR                0x0a
#define HID_REPORT_DESCRIPTOR           0x22

// EP types for EP_init
#define EP_TYPE_BULK                    0x00
#define EP_TYPE_CONTROL                 0x01
#define EP_TYPE_ISO                     0x02
#define EP_TYPE_INTERRUPT               0x03

// EP types for descriptors
#define USB_BM_ATTR_CONTROL             0x00
#define USB_BM_ATTR_ISO                 0x01
#define USB_BM_ATTR_BULK                0x02
#define USB_BM_ATTR_INTERRUPT           0x03


/******************************************************************
 *                 Other stuff                                    *
 *****************************************************************/

#define RX_FLAG(epstat)                 (epstat & USB_EPnR_CTR_RX)
#define TX_FLAG(epstat)                 (epstat & USB_EPnR_CTR_TX)
#define SETUP_FLAG(epstat)              (epstat & USB_EPnR_SETUP)

// EPnR bits manipulation
#define KEEP_DTOG_STAT(EPnR)            (EPnR & ~(USB_EPnR_STAT_RX|USB_EPnR_STAT_TX|USB_EPnR_DTOG_RX|USB_EPnR_DTOG_TX))
#define KEEP_DTOG(EPnR)                 (EPnR & ~(USB_EPnR_DTOG_RX|USB_EPnR_DTOG_TX))

#define LANG_US (uint16_t)0x0409

#define _USB_STRING_(name, str)                  \
static const struct name \
{                          \
        uint8_t  bLength;                       \
        uint8_t  bDescriptorType;               \
        uint16_t bString[(sizeof(str) - 2) / 2]; \
    \
} \
name = {sizeof(name), 0x03, str}

#define _USB_LANG_ID_(name, lng_id)     \
static const struct name \
{         \
        uint8_t  bLength;         \
        uint8_t  bDescriptorType; \
        uint16_t bString;         \
    \
} \
name = {0x04, 0x03, lng_id}

// EP0 configuration packet
typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} config_pack_t;

// endpoints state
typedef struct{
    uint16_t *tx_buf;           // transmission buffer address
    uint16_t txbufsz;           // transmission buffer size
    uint8_t *rx_buf;            // reception buffer address
    void (*func)();             // endpoint action function
    unsigned rx_cnt  : 10;      // received data counter
} ep_t;

extern volatile uint8_t usbON;

void USB_setup();
int EP_Init(uint8_t number, uint8_t type, uint16_t txsz, uint16_t rxsz, void (*func)());
void EP_WriteIRQ(uint8_t number, const uint8_t *buf, uint16_t size);
void EP_Write(uint8_t number, const uint8_t *buf, uint16_t size);
int EP_Read(uint8_t number, uint8_t *buf);

// could be [re]defined in usb_dev.c
extern void usb_class_request(config_pack_t *packet, uint8_t *data, uint16_t datalen);
extern void usb_vendor_request(config_pack_t *packet, uint8_t *data, uint16_t datalen);
extern void set_configuration();
