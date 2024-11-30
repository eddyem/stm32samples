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

#include "usb_descr.h"
#include "usb_lib.h"

// string descriptors
enum{
    iLANGUAGE_DESCR,
    iMANUFACTURER_DESCR,
    iPRODUCT_DESCR,
    iSERIAL_DESCR,
    iINTERFACE_DESCR1,
    iINTERFACE_DESCR2,
    iINTERFACE_DESCR3,
    iINTERFACE_DESCR4,
    iINTERFACE_DESCR5,
    iINTERFACE_DESCR6,
    iINTERFACE_DESCR7,
    iDESCR_AMOUNT
};

const uint8_t USB_DeviceDescriptor[] = {
    USB_DT_DEVICE_SIZE, // bLength
    USB_DT_DEVICE, // bDescriptorType
    L16(bcdUSB), // bcdUSB_L
    H16(bcdUSB), // bcdUSB_H
    USB_CLASS_VENDOR_SPEC, // bDeviceClass
    bDeviceSubClass, // bDeviceSubClass
    bDeviceProtocol, // bDeviceProtocol
    USB_EP0BUFSZ, // bMaxPacketSize
    L(idVendor), // idVendor_L
    H(idVendor), // idVendor_H
    L(idProduct), // idProduct_L
    H(idProduct), // idProduct_H
    L(bcdDevice_Ver), // bcdDevice_Ver_L
    H(bcdDevice_Ver), // bcdDevice_Ver_H
    iMANUFACTURER_DESCR, // iManufacturer - indexes of string descriptors in array
    iPRODUCT_DESCR, // iProduct
    iSERIAL_DESCR, // iSerial
    bNumConfigurations // bNumConfigurations
};

const uint8_t USB_DeviceQualifierDescriptor[] = {
    USB_DT_QUALIFIER_SIZE, //bLength
    USB_DT_QUALIFIER, // bDescriptorType
    L16(bcdUSB), // bcdUSB_L
    H16(bcdUSB), // bcdUSB_H
    USB_CLASS_VENDOR_SPEC, // bDeviceClass
    bDeviceSubClass, // bDeviceSubClass
    bDeviceProtocol, // bDeviceProtocol
    USB_EP0BUFSZ, // bMaxPacketSize0
    bNumConfigurations, // bNumConfigurations
    0 // Reserved
};

#define wTotalLength  (USB_DT_CONFIG_SIZE + (bNumInterfaces * USB_DT_INTERFACE_SIZE) + (bTotNumEndpoints * USB_DT_ENDPOINT_SIZE))

const uint8_t USB_ConfigDescriptor[] = {
    /*Configuration Descriptor*/
    USB_DT_CONFIG_SIZE, /* bLength: Configuration Descriptor size */
    USB_DT_CONFIG, /* bDescriptorType: Configuration */
    L16(wTotalLength),   /* wTotalLength.L :no of returned bytes */
    H16(wTotalLength), /* wTotalLength.H */
    bNumInterfaces, /* bNumInterfaces */
    1, /* bConfigurationValue: Current configuration value */
    0, /* iConfiguration: Index of string descriptor describing the configuration or 0 */
    BusPowered, /* bmAttributes - Bus powered */
    100, /* MaxPower in 2mA units */
    /*---------------------------------------------------------------------------*/
    /*Interface Descriptor */
    USB_DT_INTERFACE_SIZE, /* bLength: Interface Descriptor size */
    USB_DT_INTERFACE, /* bDescriptorType: Interface */
    0, /* bInterfaceNumber: Number of Interface */
    0, /* bAlternateSetting: Alternate setting */
    1, /* bNumEndpoints: only 1 used */
    0, /* bInterfaceClass */
    0, /* bInterfaceSubClass */
    0, /* bInterfaceProtocol */
    iINTERFACE_DESCR1, /* iInterface: */
};

_USB_LANG_ID_(LD, LANG_US);
_USB_STRING_(SD, u"0.0.1");
_USB_STRING_(MD, u"eddy@sao");
_USB_STRING_(PD, u"Some USB device");
_USB_STRING_(ID, u"first interface");

const uint8_t* const StringDescriptor[iDESCR_AMOUNT] = {
    [iLANGUAGE_DESCR] = &LD,
    [iMANUFACTURER_DESCR] = &MD,
    [iPRODUCT_DESCR] = &PD,
    [iSERIAL_DESCR] = &SD,
    [iINTERFACE_DESCR1] = &ID
};

