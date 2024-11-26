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

#include <wchar.h>
#include "usbhw.h"

#define EP0DATABUF_SIZE                 (64)
#define LASTADDR_DEFAULT                (STM32ENDPOINTS * 8)

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

// deprecated defines:
#define STANDARD_DEVICE_REQUEST_TYPE    0
#define STANDARD_INTERFACE_REQUEST_TYPE 1
#define STANDARD_ENDPOINT_REQUEST_TYPE  2
#define STANDARD_OTHER_REQUEST_TYPE     3
#define VENDOR_REQUEST_TYPE             0x40
#define CONTROL_REQUEST_TYPE            0x21

// bRequest, standard; for bmRequestType == 0x80
#define GET_STATUS                      0x00
#define GET_DESCRIPTOR                  0x06
#define GET_CONFIGURATION               0x08
// for bmRequestType == 0
#define CLEAR_FEATURE                   0x01
#define SET_FEATURE                     0x03
#define SET_ADDRESS                     0x05
#define SET_DESCRIPTOR                  0x07
#define SET_CONFIGURATION               0x09
// for bmRequestType == 0x81, 1 or 0xB2
#define GET_INTERFACE                   0x0A
#define SET_INTERFACE                   0x0B
#define SYNC_FRAME                      0x0C
#define VENDOR_REQUEST                  0x01

// string descriptors
enum{
    iLANGUAGE_DESCR,
    iMANUFACTURER_DESCR,
    iPRODUCT_DESCR,
    iSERIAL_DESCR,
    iINTERFACE_DESCR,
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
    void (*func)();             // endpoint action function
    unsigned rx_cnt  : 10;      // received data counter
} ep_t;

extern volatile uint8_t usbON;

int EP_Init(uint8_t number, uint8_t type, uint16_t txsz, uint16_t rxsz, void (*func)());
void EP_WriteIRQ(uint8_t number, const uint8_t *buf, uint16_t size);
void EP_Write(uint8_t number, const uint8_t *buf, uint16_t size);
int EP_Read(uint8_t number, uint8_t *buf);

// could be [re]defined in usbdev.c
extern void usb_class_request(config_pack_t *packet, uint8_t *data, unsigned int datalen);
extern void usb_vendor_request(config_pack_t *packet, uint8_t *data, unsigned int datalen);
extern void set_configuration(uint16_t configuration);
