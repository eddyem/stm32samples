/*
 * This file is part of the SevenCDCs project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <wchar.h>
#include "usbhw.h"

#define EP0DATABUF_SIZE                 (64)
#define LASTADDR_DEFAULT                (STM32ENDPOINTS * 8)

// bmRequestType & 0x7f
#define STANDARD_DEVICE_REQUEST_TYPE    0
#define STANDARD_ENDPOINT_REQUEST_TYPE  2
#define VENDOR_REQUEST_TYPE             0x40
#define CONTROL_REQUEST_TYPE            0x21
// bRequest, standard; for bmRequestType == 0x80
#define GET_STATUS                      0x00
#define GET_DESCRIPTOR                  0x06
#define GET_CONFIGURATION               0x08
// for bmRequestType == 0
#define CLEAR_FEATURE                   0x01
#define SET_FEATURE                     0x03    // unused
#define SET_ADDRESS                     0x05
#define SET_DESCRIPTOR                  0x07    // unused
#define SET_CONFIGURATION               0x09
// for bmRequestType == 0x81, 1 or 0xB2
#define GET_INTERFACE                   0x0A    // unused
#define SET_INTERFACE                   0x0B    // unused
#define SYNC_FRAME                      0x0C    // unused
#define VENDOR_REQUEST                  0x01    // unused

// Class-Specific Control Requests
#define SEND_ENCAPSULATED_COMMAND       0x00    // unused
#define GET_ENCAPSULATED_RESPONSE       0x01    // unused
#define SET_COMM_FEATURE                0x02    // unused
#define GET_COMM_FEATURE                0x03    // unused
#define CLEAR_COMM_FEATURE              0x04    // unused
#define SET_LINE_CODING                 0x20
#define GET_LINE_CODING                 0x21
#define SET_CONTROL_LINE_STATE          0x22
#define SEND_BREAK                      0x23

// control line states
#define CONTROL_DTR                     0x01
#define CONTROL_RTS                     0x02

// string descriptors
enum{
    iLANGUAGE_DESCR,
    iMANUFACTURER_DESCR,
    iPRODUCT_DESCR,
    iSERIAL_DESCR,
    iINTERFACE_DESCR_CMD,
    iINTERFACE_DESCR_USART1,
    iINTERFACE_DESCR_USART2,
    iINTERFACE_DESCR_USART3,
    iINTERFACE_DESCR_USART4,
    iINTERFACE_DESCR_CAN,
    iINTERFACE_DESCR_DBG,
    iDESCR_AMOUNT
};

// Types of descriptors
#define DEVICE_DESCRIPTOR               0x01
#define CONFIGURATION_DESCRIPTOR        0x02
#define STRING_DESCRIPTOR               0x03
#define DEVICE_QUALIFIER_DESCRIPTOR     0x06

#define RX_FLAG(epstat)                 (epstat & USB_EPnR_CTR_RX)
#define TX_FLAG(epstat)                 (epstat & USB_EPnR_CTR_TX)
#define SETUP_FLAG(epstat)              (epstat & USB_EPnR_SETUP)

// EPnR bits manipulation
#define KEEP_DTOG_STAT(EPnR)            (EPnR & ~(USB_EPnR_STAT_RX|USB_EPnR_STAT_TX|USB_EPnR_DTOG_RX|USB_EPnR_DTOG_TX))
#define KEEP_DTOG(EPnR)                 (EPnR & ~(USB_EPnR_DTOG_RX|USB_EPnR_DTOG_TX))

// EP types
#define EP_TYPE_BULK                    0x00
#define EP_TYPE_CONTROL                 0x01
#define EP_TYPE_ISO                     0x02
#define EP_TYPE_INTERRUPT               0x03

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
    \
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
    void (*func)(uint8_t epno); // endpoint action function
    unsigned rx_cnt  : 10;      // received data counter
} ep_t;

typedef struct {
    uint32_t dwDTERate;
    uint8_t bCharFormat;
    #define USB_CDC_1_STOP_BITS   0
    #define USB_CDC_1_5_STOP_BITS 1
    #define USB_CDC_2_STOP_BITS   2
    uint8_t bParityType;
    #define USB_CDC_NO_PARITY     0
    #define USB_CDC_ODD_PARITY    1
    #define USB_CDC_EVEN_PARITY   2
    #define USB_CDC_MARK_PARITY   3
    #define USB_CDC_SPACE_PARITY  4
    uint8_t bDataBits;
} __attribute__ ((packed)) usb_LineCoding;

extern ep_t endpoints[];
// device disconnected from terminal (BIT flags!!!)
#define USBON(ifno) (usbON & (1<<ifno))
extern volatile uint8_t usbON;
extern config_pack_t *setup_packet;
extern uint8_t ep0databuf[], setupdatabuf[];

void EP0_Handler(uint8_t epno);

void EP_WriteIRQ(uint8_t number, const uint8_t *buf, uint16_t size);
void EP_Write(uint8_t number, const uint8_t *buf, uint16_t size);
int EP_Read(uint8_t number, uint8_t *buf);

