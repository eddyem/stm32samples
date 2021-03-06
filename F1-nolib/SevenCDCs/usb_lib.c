/*
 *                                                                                                  geany_encoding=koi8-r
 * usb_lib.c
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

#include <stdint.h>
#include "usb_lib.h"

ep_t endpoints[STM32ENDPOINTS];

usb_dev_t USB_Dev;
static usb_LineCoding lineCoding = {115200, 0, 0, 8};
static config_pack_t setup_packet;
static uint8_t ep0databuf[EP0DATABUF_SIZE];
static uint8_t ep0dbuflen = 0;

usb_LineCoding getLineCoding(){return lineCoding;}

uint8_t usbON = 0; // device disconnected from terminal

// definition of parts common for USB_DeviceDescriptor & USB_DeviceQualifierDescriptor
#define bcdUSB_L        0x00
#define bcdUSB_H        0x02
// Class - Misc (EF), subclass - common (2), protocol - interface association descr (1)
#define bDeviceClass    0xEF
#define bDeviceSubClass 0x02
#define bDeviceProtocol 0x01
#define bNumConfigurations 1

static const uint8_t USB_DeviceDescriptor[] = {
        18,     // bLength
        0x01,   // bDescriptorType - Device descriptor
        bcdUSB_L,   // bcdUSB_L - 1.10
        bcdUSB_H,   // bcdUSB_H
        bDeviceClass,   // bDeviceClass - USB_COMM
        bDeviceSubClass,   // bDeviceSubClass
        bDeviceProtocol,   // bDeviceProtocol
        USB_EP0_BUFSZ,   // bMaxPacketSize
  // 0483:5740 (VID:PID) - stm32 VCP
    0x83, 0x04, 0x40, 0x57,
        0x00,   // bcdDevice_Ver_L
        0x02,   // bcdDevice_Ver_H
        0x01,   // iManufacturer
        0x02,   // iProduct
        0x03,   // iSerialNumber
        bNumConfigurations    // bNumConfigurations
};

static const uint8_t USB_DeviceQualifierDescriptor[] = {
        10,     //bLength
        0x06,   // bDescriptorType - Device qualifier
        bcdUSB_L,   // bcdUSB_L
        bcdUSB_H,   // bcdUSB_H
        bDeviceClass,   // bDeviceClass
        bDeviceSubClass,   // bDeviceSubClass
        bDeviceProtocol,   // bDeviceProtocol
        USB_EP0_BUFSZ,   // bMaxPacketSize0
        bNumConfigurations,   // bNumConfigurations
        0x00    // Reserved
};

static const uint8_t USB_ConfigDescriptor[] = {
    /* Configuration Descriptor*/
    0x09, /* bLength: Configuration Descriptor size */
    0x02, /* bDescriptorType: Configuration */
    0xD7,   /* wTotalLength:no of returned bytes (9+7*66=471, 0x1D7 */
    0x01,
    14, /* bNumInterfaces: 14 interfaces (7*2) */
    0x01, /* bConfigurationValue: Configuration value */
    0x00, /* iConfiguration: Index of string descriptor describing the configuration */
    0x80, /* bmAttributes - Bus powered */
    0x32, /* MaxPower 100 mA */

    /*---------------------------------------------------------------------------*/
    // IAD0 (66 bytes)
    0x08,        // bLength: Interface Descriptor size
    0x0B,        // bDescriptorType: IAD
    0,           // bFirstInterface
    0x02,        // bInterfaceCount
    0x02,        // bFunctionClass: CDC
    0x02,        // bFunctionSubClass
    0x01,        // bFunctionProtocol (0 or 1?)
    0x00,        // Interface string index
    /*---------------------------------------------------------------------------*/
    /* Interface Descriptor */
    0x09, /* bLength: Interface Descriptor size */
    0x04, /* bDescriptorType: Interface */
    0x00, /* bInterfaceNumber: Number of Interface */
    0x00, /* bAlternateSetting: Alternate setting */
    0x01, /* bNumEndpoints: One endpoints used */
    0x02, /* bInterfaceClass: Communication Interface Class */
    0x02, /* bInterfaceSubClass: Abstract Control Model */
    0x00, /* bInterfaceProtocol */
    0x00, /* iInterface: */
    /*Header Functional Descriptor*/
    0x05, /* bLength: Endpoint Descriptor size */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x00, /* bDescriptorSubtype: Header Func Desc */
    0x10, /* bcdCDC: spec release number */
    0x01,
    /*Call Management Functional Descriptor*/
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x01, /* bDescriptorSubtype: Call Management Func Desc */
    0x00, /* bmCapabilities: D0+D1 */
    0x01, /* bDataInterface: 1 */
    /*ACM Functional Descriptor*/
    0x04, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x02, /* bDescriptorSubtype: Abstract Control Management desc */
    0x02, /* bmCapabilities */
    /*Union Functional Descriptor*/
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x06, /* bDescriptorSubtype: Union func desc */
    0x00, /* bMasterInterface: Communication class interface */
    0x01, /* bSlaveInterface0: Data Class Interface */
    /*Endpoint 10 Descriptor*/
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x88, /* bEndpointAddress IN8 - non-existant! */
    0x03, /* bmAttributes: Interrupt */
    (USB_EP1BUFSZ & 0xff), /* wMaxPacketSize LO: */
    (USB_EP1BUFSZ >> 8), /* wMaxPacketSize HI: */
    0x10, /* bInterval: */
    /*---------------------------------------------------------------------------*/
    /*Data class interface descriptor*/
    0x09, /* bLength: Endpoint Descriptor size */
    0x04, /* bDescriptorType: */
    0x01, /* bInterfaceNumber: Number of Interface */
    0x00, /* bAlternateSetting: Alternate setting */
    0x02, /* bNumEndpoints: Two endpoints used */
    0x0A, /* bInterfaceClass: CDC */
    0x02, /* bInterfaceSubClass: */
    0x00, /* bInterfaceProtocol: */
    0x00, /* iInterface: */
    /*Endpoint IN Descriptor*/
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
    0x81, /* bEndpointAddress IN1 */
    0x02, /* bmAttributes: Bulk */
    (USB_TXBUFSZ & 0xff), /* wMaxPacketSize: 64 */
    (USB_TXBUFSZ >> 8),
    0x00, /* bInterval: ignore for Bulk transfer */
    /*Endpoint OUT Descriptor*/
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
    0x01, /* bEndpointAddress OUT1 */
    0x02, /* bmAttributes: Bulk */
    (USB_TXBUFSZ & 0xff), /* wMaxPacketSize: 64 */
    (USB_TXBUFSZ >> 8),
    0x00, /* bInterval: ignore for Bulk transfer */
    /*---------------------------------------------------------------------------*/

    /*---------------------------------------------------------------------------*/
    // IAD1: interfaces 2&3, EP2
    0x08,        // bLength: Interface Descriptor size
    0x0B,        // bDescriptorType: IAD
/**/   2,        // bFirstInterface
    0x02,        // bInterfaceCount
    0x02,        // bFunctionClass: CDC
    0x02,        // bFunctionSubClass
    0x01,        // bFunctionProtocol (0 or 1?)
    0x02,        // iFunction
    /*---------------------------------------------------------------------------*/
    /*Interface Descriptor */
    0x09, /* bLength: Interface Descriptor size */
    0x04, /* bDescriptorType: Interface */
/**/   2, /* bInterfaceNumber: Number of Interface */
    0x00, /* bAlternateSetting: Alternate setting */
    0x01, /* bNumEndpoints: One endpoints used */
    0x02, /* bInterfaceClass: Communication Interface Class */
    0x02, /* bInterfaceSubClass: Abstract Control Model */
    0x00, /* bInterfaceProtocol  */
    0x00, /* iInterface: */
    /*Header Functional Descriptor*/
    0x05, /* bLength: Endpoint Descriptor size */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x00, /* bDescriptorSubtype: Header Func Desc */
    0x10, /* bcdCDC: spec release number */
    0x01,
    /*Call Management Functional Descriptor*/
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x01, /* bDescriptorSubtype: Call Management Func Desc */
    0x00, /* bmCapabilities: D0+D1 */
/**/   3, /* bDataInterface: 3 */
    /*ACM Functional Descriptor*/
    0x04, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x02, /* bDescriptorSubtype: Abstract Control Management desc */
    0x02, /* bmCapabilities */
    /*Union Functional Descriptor*/
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x06, /* bDescriptorSubtype: Union func desc */
/**/   2, /* bMasterInterface: Communication class interface */
/**/   3, /* bSlaveInterface0: Data Class Interface */
    /*Endpoint 11 Descriptor*/
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x89, /* bEndpointAddress IN9 - non-existant! */
    0x03, /* bmAttributes: Interrupt */
    (USB_EP1BUFSZ & 0xff), /* wMaxPacketSize LO: */
    (USB_EP1BUFSZ >> 8), /* wMaxPacketSize HI: */
    0x10, /* bInterval: */
    /*---------------------------------------------------------------------------*/
    /*Data class interface descriptor*/
    0x09, /* bLength: Endpoint Descriptor size */
    0x04, /* bDescriptorType: */
/**/   3,    /* bInterfaceNumber: Number of Interface */
    0x00, /* bAlternateSetting: Alternate setting */
    0x02, /* bNumEndpoints: Two endpoints used */
    0x0A, /* bInterfaceClass: CDC */
    0x02, /* bInterfaceSubClass: */
    0x00, /* bInterfaceProtocol: */
    0x00, /* iInterface: */
    /*Endpoint IN Descriptor */
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x82, /* bEndpointAddress IN2 */
    0x02, /* bmAttributes: Bulk */
    (USB_TXBUFSZ & 0xff), /* wMaxPacketSize: 64 */
    (USB_TXBUFSZ >> 8),
    0x00, /* bInterval: ignore for Bulk transfer */
    /*Endpoint OUT Descriptor */
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x02, /* bEndpointAddress OUT2 */
    0x02, /* bmAttributes: Bulk */
    (USB_TXBUFSZ & 0xff), /* wMaxPacketSize: 64 */
    (USB_TXBUFSZ >> 8),
    0x00, /* bInterval: ignore for Bulk transfer */
    /*---------------------------------------------------------------------------*/

    /*---------------------------------------------------------------------------*/
    // IAD2: interfaces 4&5, EP3
    0x08,        // bLength: Interface Descriptor size
    0x0B,        // bDescriptorType: IAD
/**/   4,        // bFirstInterface
    0x02,        // bInterfaceCount
    0x02,        // bFunctionClass: CDC
    0x02,        // bFunctionSubClass
    0x01,        // bFunctionProtocol (0 or 1?)
    0x02,        // iFunction
    /*---------------------------------------------------------------------------*/
    /*Interface Descriptor */
    0x09, /* bLength: Interface Descriptor size */
    0x04, /* bDescriptorType: Interface */
/**/   4, /* bInterfaceNumber: Number of Interface */
    0x00, /* bAlternateSetting: Alternate setting */
    0x01, /* bNumEndpoints: One endpoints used */
    0x02, /* bInterfaceClass: Communication Interface Class */
    0x02, /* bInterfaceSubClass: Abstract Control Model */
    0x00, /* bInterfaceProtocol  */
    0x00, /* iInterface: */
    /*Header Functional Descriptor*/
    0x05, /* bLength: Endpoint Descriptor size */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x00, /* bDescriptorSubtype: Header Func Desc */
    0x10, /* bcdCDC: spec release number */
    0x01,
    /*Call Management Functional Descriptor*/
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x01, /* bDescriptorSubtype: Call Management Func Desc */
    0x00, /* bmCapabilities: D0+D1 */
/**/   5, /* bDataInterface */
    /*ACM Functional Descriptor*/
    0x04, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x02, /* bDescriptorSubtype: Abstract Control Management desc */
    0x02, /* bmCapabilities */
    /*Union Functional Descriptor*/
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x06, /* bDescriptorSubtype: Union func desc */
/**/   4, /* bMasterInterface: Communication class interface */
/**/   5, /* bSlaveInterface0: Data Class Interface */
    /*Endpoint 11 Descriptor*/
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x8a, /* bEndpointAddress IN10 - non-existant! */
    0x03, /* bmAttributes: Interrupt */
    (USB_EP1BUFSZ & 0xff), /* wMaxPacketSize LO: */
    (USB_EP1BUFSZ >> 8), /* wMaxPacketSize HI: */
    0x10, /* bInterval: */
    /*---------------------------------------------------------------------------*/
    /*Data class interface descriptor*/
    0x09, /* bLength: Endpoint Descriptor size */
    0x04, /* bDescriptorType: */
/**/   5,    /* bInterfaceNumber: Number of Interface */
    0x00, /* bAlternateSetting: Alternate setting */
    0x02, /* bNumEndpoints: Two endpoints used */
    0x0A, /* bInterfaceClass: CDC */
    0x02, /* bInterfaceSubClass: */
    0x00, /* bInterfaceProtocol: */
    0x00, /* iInterface: */
    /*Endpoint IN Descriptor */
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x83, /* bEndpointAddress IN3 */
    0x02, /* bmAttributes: Bulk */
    (USB_TXBUFSZ & 0xff), /* wMaxPacketSize: 64 */
    (USB_TXBUFSZ >> 8),
    0x00, /* bInterval: ignore for Bulk transfer */
    /*Endpoint OUT Descriptor */
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x03, /* bEndpointAddress OUT3 */
    0x02, /* bmAttributes: Bulk */
    (USB_TXBUFSZ & 0xff), /* wMaxPacketSize: 64 */
    (USB_TXBUFSZ >> 8),
    0x00, /* bInterval: ignore for Bulk transfer */
    /*---------------------------------------------------------------------------*/

    /*---------------------------------------------------------------------------*/
    // IAD3: interfaces 6&7, EP4
    0x08,        // bLength: Interface Descriptor size
    0x0B,        // bDescriptorType: IAD
/**/   6,        // bFirstInterface
    0x02,        // bInterfaceCount
    0x02,        // bFunctionClass: CDC
    0x02,        // bFunctionSubClass
    0x01,        // bFunctionProtocol (0 or 1?)
    0x02,        // iFunction
    /*---------------------------------------------------------------------------*/
    /*Interface Descriptor */
    0x09, /* bLength: Interface Descriptor size */
    0x04, /* bDescriptorType: Interface */
/**/   6, /* bInterfaceNumber: Number of Interface */
    0x00, /* bAlternateSetting: Alternate setting */
    0x01, /* bNumEndpoints: One endpoints used */
    0x02, /* bInterfaceClass: Communication Interface Class */
    0x02, /* bInterfaceSubClass: Abstract Control Model */
    0x00, /* bInterfaceProtocol  */
    0x00, /* iInterface: */
    /*Header Functional Descriptor*/
    0x05, /* bLength: Endpoint Descriptor size */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x00, /* bDescriptorSubtype: Header Func Desc */
    0x10, /* bcdCDC: spec release number */
    0x01,
    /*Call Management Functional Descriptor*/
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x01, /* bDescriptorSubtype: Call Management Func Desc */
    0x00, /* bmCapabilities: D0+D1 */
/**/   7, /* bDataInterface */
    /*ACM Functional Descriptor*/
    0x04, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x02, /* bDescriptorSubtype: Abstract Control Management desc */
    0x02, /* bmCapabilities */
    /*Union Functional Descriptor*/
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x06, /* bDescriptorSubtype: Union func desc */
/**/   6, /* bMasterInterface: Communication class interface */
/**/   7, /* bSlaveInterface0: Data Class Interface */
    /* Interrupt Endpoint Descriptor */
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x8b, /* bEndpointAddress IN11 - non-existant! */
    0x03, /* bmAttributes: Interrupt */
    (USB_EP1BUFSZ & 0xff), /* wMaxPacketSize LO: */
    (USB_EP1BUFSZ >> 8), /* wMaxPacketSize HI: */
    0x10, /* bInterval: */
    /*---------------------------------------------------------------------------*/
    /*Data class interface descriptor*/
    0x09, /* bLength: Endpoint Descriptor size */
    0x04, /* bDescriptorType: */
/**/   7,    /* bInterfaceNumber: Number of Interface */
    0x00, /* bAlternateSetting: Alternate setting */
    0x02, /* bNumEndpoints: Two endpoints used */
    0x0A, /* bInterfaceClass: CDC */
    0x02, /* bInterfaceSubClass: */
    0x00, /* bInterfaceProtocol: */
    0x00, /* iInterface: */
    /*Endpoint IN Descriptor */
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x84, /* bEndpointAddress IN4 */
    0x02, /* bmAttributes: Bulk */
    (USB_TXBUFSZ & 0xff), /* wMaxPacketSize: 64 */
    (USB_TXBUFSZ >> 8),
    0x00, /* bInterval: ignore for Bulk transfer */
    /*Endpoint OUT Descriptor */
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x04, /* bEndpointAddress OUT4 */
    0x02, /* bmAttributes: Bulk */
    (USB_TXBUFSZ & 0xff), /* wMaxPacketSize: 64 */
    (USB_TXBUFSZ >> 8),
    0x00, /* bInterval: ignore for Bulk transfer */
    /*---------------------------------------------------------------------------*/

    /*---------------------------------------------------------------------------*/
    // IAD4: interfaces 8&9, EP5
    0x08,        // bLength: Interface Descriptor size
    0x0B,        // bDescriptorType: IAD
/**/   8,        // bFirstInterface
    0x02,        // bInterfaceCount
    0x02,        // bFunctionClass: CDC
    0x02,        // bFunctionSubClass
    0x01,        // bFunctionProtocol (0 or 1?)
    0x02,        // iFunction
    /*---------------------------------------------------------------------------*/
    /*Interface Descriptor */
    0x09, /* bLength: Interface Descriptor size */
    0x04, /* bDescriptorType: Interface */
/**/   8, /* bInterfaceNumber: Number of Interface */
    0x00, /* bAlternateSetting: Alternate setting */
    0x01, /* bNumEndpoints: One endpoints used */
    0x02, /* bInterfaceClass: Communication Interface Class */
    0x02, /* bInterfaceSubClass: Abstract Control Model */
    0x00, /* bInterfaceProtocol  */
    0x00, /* iInterface: */
    /*Header Functional Descriptor*/
    0x05, /* bLength: Endpoint Descriptor size */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x00, /* bDescriptorSubtype: Header Func Desc */
    0x10, /* bcdCDC: spec release number */
    0x01,
    /*Call Management Functional Descriptor*/
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x01, /* bDescriptorSubtype: Call Management Func Desc */
    0x00, /* bmCapabilities: D0+D1 */
/**/   9, /* bDataInterface */
    /*ACM Functional Descriptor*/
    0x04, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x02, /* bDescriptorSubtype: Abstract Control Management desc */
    0x02, /* bmCapabilities */
    /*Union Functional Descriptor*/
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x06, /* bDescriptorSubtype: Union func desc */
/**/   8, /* bMasterInterface: Communication class interface */
/**/   9, /* bSlaveInterface0: Data Class Interface */
    /* Interrupt Endpoint Descriptor */
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x8c, /* bEndpointAddress IN12 - non-existant! */
    0x03, /* bmAttributes: Interrupt */
    (USB_EP1BUFSZ & 0xff), /* wMaxPacketSize LO: */
    (USB_EP1BUFSZ >> 8), /* wMaxPacketSize HI: */
    0x10, /* bInterval: */
    /*---------------------------------------------------------------------------*/
    /*Data class interface descriptor*/
    0x09, /* bLength: Endpoint Descriptor size */
    0x04, /* bDescriptorType: */
/**/   9,    /* bInterfaceNumber: Number of Interface */
    0x00, /* bAlternateSetting: Alternate setting */
    0x02, /* bNumEndpoints: Two endpoints used */
    0x0A, /* bInterfaceClass: CDC */
    0x02, /* bInterfaceSubClass: */
    0x00, /* bInterfaceProtocol: */
    0x00, /* iInterface: */
    /*Endpoint IN Descriptor */
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x85, /* bEndpointAddress IN5 */
    0x02, /* bmAttributes: Bulk */
    (USB_TXBUFSZ & 0xff), /* wMaxPacketSize: 64 */
    (USB_TXBUFSZ >> 8),
    0x00, /* bInterval: ignore for Bulk transfer */
    /*Endpoint OUT Descriptor */
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x05, /* bEndpointAddress OUT5 */
    0x02, /* bmAttributes: Bulk */
    (USB_TXBUFSZ & 0xff), /* wMaxPacketSize: 64 */
    (USB_TXBUFSZ >> 8),
    0x00, /* bInterval: ignore for Bulk transfer */
    /*---------------------------------------------------------------------------*/

    /*---------------------------------------------------------------------------*/
    // IAD5: interfaces 10&11, EP6
    0x08,        // bLength: Interface Descriptor size
    0x0B,        // bDescriptorType: IAD
/**/  10,        // bFirstInterface
    0x02,        // bInterfaceCount
    0x02,        // bFunctionClass: CDC
    0x02,        // bFunctionSubClass
    0x01,        // bFunctionProtocol (0 or 1?)
    0x02,        // iFunction
    /*---------------------------------------------------------------------------*/
    /*Interface Descriptor */
    0x09, /* bLength: Interface Descriptor size */
    0x04, /* bDescriptorType: Interface */
/**/  10, /* bInterfaceNumber: Number of Interface */
    0x00, /* bAlternateSetting: Alternate setting */
    0x01, /* bNumEndpoints: One endpoints used */
    0x02, /* bInterfaceClass: Communication Interface Class */
    0x02, /* bInterfaceSubClass: Abstract Control Model */
    0x00, /* bInterfaceProtocol  */
    0x00, /* iInterface: */
    /*Header Functional Descriptor*/
    0x05, /* bLength: Endpoint Descriptor size */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x00, /* bDescriptorSubtype: Header Func Desc */
    0x10, /* bcdCDC: spec release number */
    0x01,
    /*Call Management Functional Descriptor*/
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x01, /* bDescriptorSubtype: Call Management Func Desc */
    0x00, /* bmCapabilities: D0+D1 */
/**/  11, /* bDataInterface */
    /*ACM Functional Descriptor*/
    0x04, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x02, /* bDescriptorSubtype: Abstract Control Management desc */
    0x02, /* bmCapabilities */
    /*Union Functional Descriptor*/
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x06, /* bDescriptorSubtype: Union func desc */
/**/  10, /* bMasterInterface: Communication class interface */
/**/  11, /* bSlaveInterface0: Data Class Interface */
    /* Interrupt Endpoint Descriptor */
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x8d, /* bEndpointAddress IN13 - non-existant! */
    0x03, /* bmAttributes: Interrupt */
    (USB_EP1BUFSZ & 0xff), /* wMaxPacketSize LO: */
    (USB_EP1BUFSZ >> 8), /* wMaxPacketSize HI: */
    0x10, /* bInterval: */
    /*---------------------------------------------------------------------------*/
    /*Data class interface descriptor*/
    0x09, /* bLength: Endpoint Descriptor size */
    0x04, /* bDescriptorType: */
/**/  11,    /* bInterfaceNumber: Number of Interface */
    0x00, /* bAlternateSetting: Alternate setting */
    0x02, /* bNumEndpoints: Two endpoints used */
    0x0A, /* bInterfaceClass: CDC */
    0x02, /* bInterfaceSubClass: */
    0x00, /* bInterfaceProtocol: */
    0x00, /* iInterface: */
    /*Endpoint IN Descriptor */
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x86, /* bEndpointAddress IN6 */
    0x02, /* bmAttributes: Bulk */
    (USB_TXBUFSZ & 0xff), /* wMaxPacketSize: 64 */
    (USB_TXBUFSZ >> 8),
    0x00, /* bInterval: ignore for Bulk transfer */
    /*Endpoint OUT Descriptor */
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x06, /* bEndpointAddress OUT6 */
    0x02, /* bmAttributes: Bulk */
    (USB_TXBUFSZ & 0xff), /* wMaxPacketSize: 64 */
    (USB_TXBUFSZ >> 8),
    0x00, /* bInterval: ignore for Bulk transfer */
    /*---------------------------------------------------------------------------*/

    /*---------------------------------------------------------------------------*/
    // IAD6: interfaces 12&13, EP7
    0x08,        // bLength: Interface Descriptor size
    0x0B,        // bDescriptorType: IAD
/**/  12,        // bFirstInterface
    0x02,        // bInterfaceCount
    0x02,        // bFunctionClass: CDC
    0x02,        // bFunctionSubClass
    0x01,        // bFunctionProtocol (0 or 1?)
    0x02,        // iFunction
    /*---------------------------------------------------------------------------*/
    /*Interface Descriptor */
    0x09, /* bLength: Interface Descriptor size */
    0x04, /* bDescriptorType: Interface */
/**/  12, /* bInterfaceNumber: Number of Interface */
    0x00, /* bAlternateSetting: Alternate setting */
    0x01, /* bNumEndpoints: One endpoints used */
    0x02, /* bInterfaceClass: Communication Interface Class */
    0x02, /* bInterfaceSubClass: Abstract Control Model */
    0x00, /* bInterfaceProtocol  */
    0x00, /* iInterface: */
    /*Header Functional Descriptor*/
    0x05, /* bLength: Endpoint Descriptor size */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x00, /* bDescriptorSubtype: Header Func Desc */
    0x10, /* bcdCDC: spec release number */
    0x01,
    /*Call Management Functional Descriptor*/
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x01, /* bDescriptorSubtype: Call Management Func Desc */
    0x00, /* bmCapabilities: D0+D1 */
/**/  13, /* bDataInterface */
    /*ACM Functional Descriptor*/
    0x04, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x02, /* bDescriptorSubtype: Abstract Control Management desc */
    0x02, /* bmCapabilities */
    /*Union Functional Descriptor*/
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x06, /* bDescriptorSubtype: Union func desc */
/**/  12, /* bMasterInterface: Communication class interface */
/**/  13, /* bSlaveInterface0: Data Class Interface */
    /* Interrupt Endpoint Descriptor */
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x8e, /* bEndpointAddress IN14 - non-existant! */
    0x03, /* bmAttributes: Interrupt */
    (USB_EP1BUFSZ & 0xff), /* wMaxPacketSize LO: */
    (USB_EP1BUFSZ >> 8), /* wMaxPacketSize HI: */
    0x10, /* bInterval: */
    /*---------------------------------------------------------------------------*/
    /*Data class interface descriptor*/
    0x09, /* bLength: Endpoint Descriptor size */
    0x04, /* bDescriptorType: */
/**/  13,    /* bInterfaceNumber: Number of Interface */
    0x00, /* bAlternateSetting: Alternate setting */
    0x02, /* bNumEndpoints: Two endpoints used */
    0x0A, /* bInterfaceClass: CDC */
    0x02, /* bInterfaceSubClass: */
    0x00, /* bInterfaceProtocol: */
    0x00, /* iInterface: */
    /*Endpoint IN Descriptor */
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x87, /* bEndpointAddress IN7 */
    0x02, /* bmAttributes: Bulk */
    (USB_TXBUFSZ & 0xff), /* wMaxPacketSize: 64 */
    (USB_TXBUFSZ >> 8),
    0x00, /* bInterval: ignore for Bulk transfer */
    /*Endpoint OUT Descriptor */
    0x07, /* bLength: Endpoint Descriptor size */
    0x05, /* bDescriptorType: Endpoint */
/**/0x07, /* bEndpointAddress OUT7 */
    0x02, /* bmAttributes: Bulk */
    (USB_TXBUFSZ & 0xff), /* wMaxPacketSize: 64 */
    (USB_TXBUFSZ >> 8),
    0x00, /* bInterval: ignore for Bulk transfer */
    /*---------------------------------------------------------------------------*/
};


USB_LANG_ID(USB_StringLangDescriptor, LANG_US);

USB_STRING(USB_StringSerialDescriptor, u"000001");
USB_STRING(USB_StringManufacturingDescriptor, u"Eddy @ SAO RAS");
USB_STRING(USB_StringProdDescriptor, u"USB-Serial Controller");

/*
 * default handlers
 */
// SET_LINE_CODING
void WEAK linecoding_handler(usb_LineCoding __attribute__((unused)) *lcd){
    //MSG("linecoding_handler()");
}

// SET_CONTROL_LINE_STATE
void WEAK clstate_handler(uint16_t __attribute__((unused)) val){
    //MSG("clstate_handler()");
}

// SEND_BREAK
void WEAK break_handler(){
    //MSG("break_handler()");
}

static void wr0(const uint8_t *buf, uint16_t size){
    if(setup_packet.wLength < size) size = setup_packet.wLength; // shortened request
    if(size < endpoints[0].txbufsz){
        EP_WriteIRQ(0, buf, size);
        return;
    }
    while(size){
        uint16_t l = size;
        if(l > endpoints[0].txbufsz) l = endpoints[0].txbufsz;
        EP_WriteIRQ(0, buf, l);
        buf += l;
        size -= l;
        uint8_t needzlp = (l == endpoints[0].txbufsz) ? 1 : 0;
        if(size || needzlp){ // send last data buffer
            uint16_t status = KEEP_DTOG(USB->EPnR[0]);
            // keep DTOGs, clear CTR_RX,TX, set TX VALID, leave stat_Rx
            USB->EPnR[0] = (status & ~(USB_EPnR_CTR_RX|USB_EPnR_CTR_TX|USB_EPnR_STAT_RX))
                            ^ USB_EPnR_STAT_TX;
            uint32_t ctr = 1000000;
            while(--ctr && (USB->ISTR & USB_ISTR_CTR) == 0){IWDG->KR = IWDG_REFRESH;};
            if((USB->ISTR & USB_ISTR_CTR) == 0){
                return;
            }
            if(needzlp) EP_WriteIRQ(0, (uint8_t*)0, 0);
        }
    }
}

static inline void get_descriptor(){
    switch(setup_packet.wValue){
        case DEVICE_DESCRIPTOR:
            wr0(USB_DeviceDescriptor, sizeof(USB_DeviceDescriptor));
        break;
        case CONFIGURATION_DESCRIPTOR:
            wr0(USB_ConfigDescriptor, sizeof(USB_ConfigDescriptor));
        break;
        case STRING_LANG_DESCRIPTOR:
            wr0((const uint8_t *)&USB_StringLangDescriptor, STRING_LANG_DESCRIPTOR_SIZE_BYTE);
        break;
        case STRING_MAN_DESCRIPTOR:
            wr0((const uint8_t *)&USB_StringManufacturingDescriptor, USB_StringManufacturingDescriptor.bLength);
        break;
        case STRING_PROD_DESCRIPTOR:
            wr0((const uint8_t *)&USB_StringProdDescriptor, USB_StringProdDescriptor.bLength);
        break;
        case STRING_SN_DESCRIPTOR:
            wr0((const uint8_t *)&USB_StringSerialDescriptor, USB_StringSerialDescriptor.bLength);
        break;
        case DEVICE_QUALIFIER_DESCRIPTOR:
            wr0(USB_DeviceQualifierDescriptor, USB_DeviceQualifierDescriptor[0]);
        break;
        default:
        break;
    }
}

static uint8_t configuration = 0; // reply for GET_CONFIGURATION (==1 if configured)
static inline void std_d2h_req(){
    uint16_t state = 0; // bus powered
    switch(setup_packet.bRequest){
        case GET_DESCRIPTOR:
            get_descriptor();
        break;
        case GET_STATUS:
            EP_WriteIRQ(0, (uint8_t *)&state, 2); // send status: Bus Powered
        break;
        case GET_CONFIGURATION:
            EP_WriteIRQ(0, &configuration, 1);
        break;
        default:
        break;
    }
}

static inline void std_h2d_req(){
    switch(setup_packet.bRequest){
        case SET_ADDRESS:
            // new address will be assigned later - after  acknowlegement or request to host
            USB_Dev.USB_Addr = setup_packet.wValue;
        break;
        case SET_CONFIGURATION:
            // Now device configured
            USB_Dev.USB_Status = USB_STATE_CONFIGURED;
            configuration = setup_packet.wValue;
        break;
        default:
        break;
    }
}

/*
bmRequestType: 76543210
7    direction: 0 - host->device, 1 - device->host
65   type: 0 - standard, 1 - class, 2 - vendor
4..0 getter: 0 - device, 1 - interface, 2 - endpoint, 3 - other
*/
/**
 * Endpoint0 (control) handler
 */
static void EP0_Handler(uint8_t epno){
    (void) epno;
    uint8_t reqtype = setup_packet.bmRequestType & 0x7f;
    uint8_t dev2host = (setup_packet.bmRequestType & 0x80) ? 1 : 0;
    uint16_t epstatus = USB->EPnR[0];
    int rxflag = RX_FLAG(epstatus);
    if(rxflag && SETUP_FLAG(epstatus)){
        switch(reqtype){
            case STANDARD_DEVICE_REQUEST_TYPE: // standard device request
                if(dev2host){
                    std_d2h_req();
                }else{
                    std_h2d_req();
                    EP_WriteIRQ(0, (uint8_t *)0, 0);
                }
            break;
            case STANDARD_ENDPOINT_REQUEST_TYPE: // standard endpoint request
                if(setup_packet.bRequest == CLEAR_FEATURE){
                    EP_WriteIRQ(0, (uint8_t *)0, 0);
                }else{
                }
            break;
            case CONTROL_REQUEST_TYPE:
                switch(setup_packet.bRequest){
                    case GET_LINE_CODING:
                        EP_WriteIRQ(0, (uint8_t*)&lineCoding, sizeof(lineCoding));
                    break;
                    case SET_LINE_CODING: // omit this for next stage, when data will come
                    break;
                    case SET_CONTROL_LINE_STATE:
                        usbON = 1;
                        clstate_handler(setup_packet.wValue);
                    break;
                    case SEND_BREAK:
                        break_handler();
                    break;
                    default:
                    break;
                }
                if(setup_packet.bRequest != GET_LINE_CODING) EP_WriteIRQ(0, (uint8_t *)0, 0);
            break;
            default:
                EP_WriteIRQ(0, (uint8_t *)0, 0);
        }
    }else if(rxflag){ // got data over EP0 or host acknowlegement
        if(endpoints[0].rx_cnt){
            if(setup_packet.bRequest == SET_LINE_CODING){
                linecoding_handler((usb_LineCoding*)ep0databuf);
            }
        }
    }else if(TX_FLAG(epstatus)){ // package transmitted
        // now we can change address after enumeration
        if((USB->DADDR & USB_DADDR_ADD) != USB_Dev.USB_Addr){
            USB->DADDR = USB_DADDR_EF | USB_Dev.USB_Addr;
            // change state to ADRESSED
            USB_Dev.USB_Status = USB_STATE_ADDRESSED;
        }
    }
    epstatus = KEEP_DTOG(USB->EPnR[0]);
    if(rxflag) epstatus ^= USB_EPnR_STAT_TX; // start ZLP/data transmission
    else epstatus &= ~USB_EPnR_STAT_TX; // or leave unchanged
    // keep DTOGs, clear CTR_RX,TX, set RX VALID
    USB->EPnR[0] = (epstatus & ~(USB_EPnR_CTR_RX|USB_EPnR_CTR_TX)) ^ USB_EPnR_STAT_RX;
}

static uint16_t lastaddr = LASTADDR_DEFAULT;
/**
 * Endpoint initialisation
 * @param number - EP num (0...7)
 * @param type - EP type (EP_TYPE_BULK, EP_TYPE_CONTROL, EP_TYPE_ISO, EP_TYPE_INTERRUPT)
 * @param txsz - transmission buffer size @ USB/CAN buffer
 * @param rxsz - reception buffer size @ USB/CAN buffer
 * @param uint16_t (*func)(uint8_t epno) - EP handler function
 * @return 0 if all OK
 */
int EP_Init(uint8_t number, uint8_t type, uint16_t txsz, uint16_t rxsz, void (*func)(uint8_t epno)){
    if(number >= STM32ENDPOINTS) return 4; // out of configured amount
    if(txsz > USB_BTABLE_SIZE || rxsz > USB_BTABLE_SIZE) return 1; // buffer too large
    if(lastaddr + txsz + rxsz > USB_BTABLE_SIZE+1) return 2; // out of btable
    USB->EPnR[number] = (type << 9) | (number & USB_EPnR_EA);
    USB->EPnR[number] ^= USB_EPnR_STAT_RX | USB_EPnR_STAT_TX_1;
    if(rxsz & 1 || rxsz > 512) return 3; // wrong rx buffer size
    uint16_t countrx = 0;
    if(rxsz < 64) countrx = rxsz / 2;
    else{
        if(rxsz & 0x1f) return 3; // should be multiple of 32
        countrx = 31 + rxsz / 32;
    }
    USB_BTABLE->EP[number].USB_ADDR_TX = lastaddr;
    endpoints[number].tx_buf = (uint16_t *)(USB_BTABLE_BASE + lastaddr*2);
    endpoints[number].txbufsz = txsz;
    lastaddr += txsz;
    USB_BTABLE->EP[number].USB_COUNT_TX = 0;
    USB_BTABLE->EP[number].USB_ADDR_RX = lastaddr;
    endpoints[number].rx_buf = (uint16_t *)(USB_BTABLE_BASE + lastaddr*2);
    lastaddr += rxsz;
    USB_BTABLE->EP[number].USB_COUNT_RX = countrx << 10;
    endpoints[number].func = func;
    return 0;
}

//extern int8_t dump;
// standard IRQ handler
void usb_lp_can_rx0_isr(){
    if(USB->ISTR & USB_ISTR_RESET){
        usbON = 0;
        // Reinit registers
        USB->CNTR = USB_CNTR_RESETM | USB_CNTR_CTRM | USB_CNTR_SUSPM | USB_CNTR_WKUPM;
        // Endpoint 0 - CONTROL
        // ON USB LS size of EP0 may be 8 bytes, but on FS it should be 64 bytes!
        lastaddr = LASTADDR_DEFAULT;
        // clear address, leave only enable bit
        USB->DADDR = USB_DADDR_EF;
        // state is default - wait for enumeration
        USB_Dev.USB_Status = USB_STATE_DEFAULT;
        USB->ISTR = ~USB_ISTR_RESET;
        if(EP_Init(0, EP_TYPE_CONTROL, USB_EP0_BUFSZ, USB_EP0_BUFSZ, EP0_Handler)){
            return;
        }
    }
    if(USB->ISTR & USB_ISTR_CTR){
        // EP number
        uint8_t n = USB->ISTR & USB_ISTR_EPID;
        // copy status register
        uint16_t epstatus = USB->EPnR[n];
        // copy received bytes amount
        endpoints[n].rx_cnt = USB_BTABLE->EP[n].USB_COUNT_RX & 0x3FF; // low 10 bits is counter
        // check direction
        if(USB->ISTR & USB_ISTR_DIR){ // OUT interrupt - receive data, CTR_RX==1 (if CTR_TX == 1 - two pending transactions: receive following by transmit)
            if(n == 0){ // control endpoint
                if(epstatus & USB_EPnR_SETUP){ // setup packet -> copy data to conf_pack
                    EP_Read(0, (uint16_t*)&setup_packet);
                    ep0dbuflen = 0;
                    // interrupt handler will be called later
                }else if(epstatus & USB_EPnR_CTR_RX){ // data packet -> push received data to ep0databuf
                    ep0dbuflen = endpoints[0].rx_cnt;
                    EP_Read(0, (uint16_t*)&ep0databuf);
                }
            }
        }else{ // IN interrupt - transmit data, only CTR_TX == 1
            // enumeration end could be MSG (if EP0)
        }
        // call EP handler
        if(endpoints[n].func) endpoints[n].func(n);
    }
    if(USB->ISTR & USB_ISTR_SUSP){ // suspend -> still no connection, may sleep
        usbON = 0;
        USB->CNTR |= USB_CNTR_FSUSP | USB_CNTR_LP_MODE;
        USB->ISTR = ~USB_ISTR_SUSP;
    }
    if(USB->ISTR & USB_ISTR_WKUP){ // wakeup
        USB->CNTR &= ~(USB_CNTR_FSUSP | USB_CNTR_LP_MODE); // clear suspend flags
        USB->ISTR = ~USB_ISTR_WKUP;
    }
}

/**
 * Write data to EP buffer (called from IRQ handler)
 * @param number - EP number
 * @param *buf - array with data
 * @param size - its size
 */
void EP_WriteIRQ(uint8_t number, const uint8_t *buf, uint16_t size){
    uint8_t i;
    if(size > endpoints[number].txbufsz) size = endpoints[number].txbufsz;
    uint16_t N2 = (size + 1) >> 1;
    // the buffer is 16-bit, so we should copy data as it would be uint16_t
    uint16_t *buf16 = (uint16_t *)buf;
    uint32_t *out = (uint32_t *)endpoints[number].tx_buf;
    for(i = 0; i < N2; ++i, ++out){
        *out = buf16[i];
    }
    USB_BTABLE->EP[number].USB_COUNT_TX = size;
}

/**
 * Write data to EP buffer (called outside IRQ handler)
 * @param number - EP number
 * @param *buf - array with data
 * @param size - its size
 */
void EP_Write(uint8_t number, const uint8_t *buf, uint16_t size){
    EP_WriteIRQ(number, buf, size);
    uint16_t status = KEEP_DTOG(USB->EPnR[number]);
    // keep DTOGs, clear CTR_TX & set TX VALID to start transmission
    USB->EPnR[number] = (status & ~(USB_EPnR_CTR_TX)) ^ USB_EPnR_STAT_TX;
}

/*
 * Copy data from EP buffer into user buffer area
 * @param *buf - user array for data
 * @return amount of data read
 */
int EP_Read(uint8_t number, uint16_t *buf){
    int sz = endpoints[number].rx_cnt;
    if(!sz) return 0;
    endpoints[number].rx_cnt = 0;
    int n = (sz + 1) >> 1;
    uint32_t *in = (uint32_t *)endpoints[number].rx_buf;
    if(n){
        for(int i = 0; i < n; ++i, ++in)
            buf[i] = *(uint16_t*)in;
    }
    return sz;
}
