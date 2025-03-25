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

#undef DBG
#define DBG(x)
#undef DBGs
#define DBGs(x)

// low/high for uint16_t
#define L16(x)              (x & 0xff)
#define H16(x)              (x >> 8)

static const uint8_t USB_DeviceDescriptor[] = {
    USB_DT_DEVICE_SIZE, // bLength
    USB_DT_DEVICE, // bDescriptorType
    L16(bcdUSB), // bcdUSB_L
    H16(bcdUSB), // bcdUSB_H
    USB_CLASS_MISC, // bDeviceClass
    bDeviceSubClass, // bDeviceSubClass
    bDeviceProtocol, // bDeviceProtocol
    USB_EP0BUFSZ, // bMaxPacketSize
    L16(idVendor), // idVendor_L
    H16(idVendor), // idVendor_H
    L16(idProduct), // idProduct_L
    H16(idProduct), // idProduct_H
    L16(bcdDevice_Ver), // bcdDevice_Ver_L
    H16(bcdDevice_Ver), // bcdDevice_Ver_H
    iMANUFACTURER_DESCR, // iManufacturer - indexes of string descriptors in array
    iPRODUCT_DESCR, // iProduct
    iSERIAL_DESCR, // iSerial
    bNumConfigurations // bNumConfigurations
};

static const uint8_t USB_DeviceQualifierDescriptor[] = {
    USB_DT_QUALIFIER_SIZE, //bLength
    USB_DT_QUALIFIER, // bDescriptorType
    L16(bcdUSB), // bcdUSB_L
    H16(bcdUSB), // bcdUSB_H
    USB_CLASS_PER_INTERFACE, // bDeviceClass
    bDeviceSubClass, // bDeviceSubClass
    bDeviceProtocol, // bDeviceProtocol
    USB_EP0BUFSZ, // bMaxPacketSize0
    bNumConfigurations, // bNumConfigurations
    0 // Reserved
};

#define wTotalLength  (USB_DT_CONFIG_SIZE + bTotNumEndpoints * 66)

/*
 *  _1stI - number of first interface
 *  IDidx - 0 or index of string descriptor for given interface
 *  EPx - number of virtual interrupt EP
 *  EP - number of real EP in/out
 */
#define USB_IAD(_1stI, IDidx, EPx, EP)  \
    USB_DT_IAD_SIZE, /* bLength: IAD size */ \
    USB_DT_IAD, /* bDescriptorType: IAD */ \
    _1stI, /* bFirstInterface */ \
    2, /* bInterfaceCount */ \
    2, /* bFunctionClass: CDC */ \
    2, /* bFunctionSubClass */ \
    0, /* bFunctionProtocol */ \
    0, /* iFunction */ \
    /* Interface Descriptor */ \
    USB_DT_INTERFACE_SIZE, /* bLength: Interface Descriptor size */ \
    USB_DT_INTERFACE, /* bDescriptorType: Interface */ \
    _1stI, /* bInterfaceNumber: Number of Interface */ \
    0, /* bAlternateSetting: Alternate setting */ \
    1, /* bNumEndpoints: one for this */ \
    USB_CLASS_COMM, /* bInterfaceClass */ \
    2, /* bInterfaceSubClass: ACM */ \
    0, /* bInterfaceProtocol */ \
    IDidx, /* iInterface */ \
    /* CDC descriptor */ \
    USB_DT_CS_INTERFACE_SIZE, /* bLength */ \
    USB_DT_CS_INTERFACE, /* bDescriptorType: CS_INTERFACE */ \
    0, /* bDescriptorSubtype: Header Func Desc */ \
    0x10, /* bcdCDC: spec release number */ \
    1, /* bDataInterface */ \
    USB_DT_CS_INTERFACE_SIZE, /* bLength */ \
    USB_DT_CS_INTERFACE, /* bDescriptorType: CS_INTERFACE */ \
    1, /* bDescriptorSubtype: Call Management Func Desc */ \
    0, /* bmCapabilities: D0+D1 */ \
    (_1stI+1), /* bDataInterface */ \
    USB_DT_CS_INTERFACE_SIZE-1, /* bLength */ \
    USB_DT_CS_INTERFACE, /* bDescriptorType: CS_INTERFACE */ \
    2, /* bDescriptorSubtype: Abstract Control Management desc */ \
    2, /* bmCapabilities */ \
    USB_DT_CS_INTERFACE_SIZE, /* bLength */ \
    USB_DT_CS_INTERFACE, /* bDescriptorType: CS_INTERFACE */ \
    6, /* bDescriptorSubtype: Union func desc */ \
    _1stI, /* bMasterInterface: Communication class interface */ \
    (_1stI+1), /* bSlaveInterface0: Data Class Interface */ \
    /* Virtual endpoint 1 Descriptor */ \
    USB_DT_ENDPOINT_SIZE, /* bLength: Endpoint Descriptor size */ \
    USB_DT_ENDPOINT, /* bDescriptorType: Endpoint */ \
    (0x80+EPx), /* bEndpointAddress IN8 - non-existant */ \
    USB_BM_ATTR_INTERRUPT, /* bmAttributes: Interrupt */ \
    L16(USB_EP1BUFSZ), /* wMaxPacketSize LO */ \
    H16(USB_EP1BUFSZ), /* wMaxPacketSize HI */ \
    0x10, /* bInterval: 16ms */ \
    /* DATA endpoint */ \
    USB_DT_INTERFACE_SIZE, /* bLength: Interface Descriptor size */ \
    USB_DT_INTERFACE, /* bDescriptorType: Interface */ \
    (_1stI+1), /* bInterfaceNumber: Number of Interface */ \
    0, /* bAlternateSetting: Alternate setting */ \
    2, /* bNumEndpoints: in and out */ \
    USB_CLASS_DATA, /* bInterfaceClass */ \
    2, /* bInterfaceSubClass: ACM */ \
    0, /* bInterfaceProtocol */ \
    0, /* iInterface */ \
    /*Endpoint IN1 Descriptor */ \
    USB_DT_ENDPOINT_SIZE, /* bLength: Endpoint Descriptor size */ \
    USB_DT_ENDPOINT, /* bDescriptorType: Endpoint */ \
    (0x80+EP), /* bEndpointAddress: IN1 */ \
    USB_BM_ATTR_BULK, /* bmAttributes: Bulk */ \
    L16(USB_TXBUFSZ), /* wMaxPacketSize LO */ \
    H16(USB_TXBUFSZ), /* wMaxPacketSize HI */ \
    0, /* bInterval: ignore for Bulk transfer */ \
    /* Endpoint OUT1 Descriptor */ \
    USB_DT_ENDPOINT_SIZE, /* bLength: Endpoint Descriptor size */ \
    USB_DT_ENDPOINT, /* bDescriptorType: Endpoint */ \
    EP, /* bEndpointAddress: OUT1 */ \
    USB_BM_ATTR_BULK, /* bmAttributes: Bulk */ \
    L16(USB_RXBUFSZ), /* wMaxPacketSize LO */ \
    H16(USB_RXBUFSZ), /* wMaxPacketSize HI */ \
    0 /* bInterval: ignore for Bulk transfer */


static const uint8_t USB_ConfigDescriptor[] = {
    // Configuration Descriptor
    USB_DT_CONFIG_SIZE, // bLength: Configuration Descriptor size
    USB_DT_CONFIG, // bDescriptorType: Configuration
    L16(wTotalLength),   // wTotalLength.L :no of returned bytes
    H16(wTotalLength), // wTotalLength.H
    bNumInterfaces, // bNumInterfaces
    1, // bConfigurationValue: Current configuration value
    0, // iConfiguration: Index of string descriptor describing the configuration or 0
    BusPowered, // bmAttributes - Bus powered
    50, // MaxPower in 2mA units
    //--IADs-------------------------------------------------------------------------
    USB_IAD(0, iINTERFACE_DESCR1, 8, 1),
    USB_IAD(2, iINTERFACE_DESCR2, 9, 2),
    USB_IAD(4, iINTERFACE_DESCR3, 10, 3),
};

//const uint8_t HID_ReportDescriptor[];

_USB_LANG_ID_(LD, LANG_US);
_USB_STRING_(SD, u"0.0.1");
_USB_STRING_(MD, u"eddy@sao.ru");
_USB_STRING_(PD, u"USB BISS-C encoders controller");
_USB_STRING_(ID1, u"encoder_cmd");
_USB_STRING_(ID2, u"encoder_X");
_USB_STRING_(ID3, u"encoder_Y");

static const void* const StringDescriptor[iDESCR_AMOUNT] = {
    [iLANGUAGE_DESCR] = &LD,
    [iMANUFACTURER_DESCR] = &MD,
    [iPRODUCT_DESCR] = &PD,
    [iSERIAL_DESCR] = &SD,
    [iINTERFACE_DESCR1] = &ID1,
    [iINTERFACE_DESCR2] = &ID2,
    [iINTERFACE_DESCR3] = &ID3,
};

static void wr0(const uint8_t *buf, uint16_t size, uint16_t askedsize){
    if(askedsize < size) size = askedsize; // shortened request
    if(size < USB_EP0BUFSZ){
        EP_WriteIRQ(0, buf, size);
        DBG("short wr0");
        DBGs(uhex2str(size));
        return;
    }
    DBG("long wr0");
    DBGs(uhex2str(size));
    while(size){
        uint16_t l = size;
        if(l > USB_EP0BUFSZ) l = USB_EP0BUFSZ;
        EP_WriteIRQ(0, buf, l);
        buf += l;
        size -= l;
        uint8_t needzlp = (l == USB_EP0BUFSZ) ? 1 : 0;
        if(size || needzlp){ // send last data buffer
            uint16_t epstatus = KEEP_DTOG(USB->EPnR[0]);
            // keep DTOGs, clear CTR_RX,TX, set TX VALID, leave stat_Rx
            USB->EPnR[0] = (epstatus & ~(USB_EPnR_CTR_RX|USB_EPnR_CTR_TX|USB_EPnR_STAT_RX))
                           ^ USB_EPnR_STAT_TX;
            uint32_t ctr = 1000000;
            while(--ctr && (USB->ISTR & USB_ISTR_CTR) == 0){IWDG->KR = IWDG_REFRESH;};
            if((USB->ISTR & USB_ISTR_CTR) == 0){
                return;
            }
            if(needzlp) EP_WriteIRQ(0, NULL, 0);
        }
    }
}

void get_descriptor(config_pack_t *pack){
    uint8_t descrtype = pack->wValue >> 8,
        descridx = pack->wValue & 0xff;
    switch(descrtype){
        case DEVICE_DESCRIPTOR:
            DBG("DEVICE_DESCRIPTOR");
            wr0(USB_DeviceDescriptor, sizeof(USB_DeviceDescriptor), pack->wLength);
            break;
        case CONFIGURATION_DESCRIPTOR:
            DBG("CONFIGURATION_DESCRIPTOR");
            wr0(USB_ConfigDescriptor, sizeof(USB_ConfigDescriptor), pack->wLength);
            break;
        case STRING_DESCRIPTOR:
            DBG("STRING_DESCRIPTOR");
            if(descridx < iDESCR_AMOUNT){
                wr0((const uint8_t *)StringDescriptor[descridx], *((uint8_t*)StringDescriptor[descridx]), pack->wLength);
                DBGs(uhex2str(descridx));
            }else{
                EP_WriteIRQ(0, NULL, 0);
                DBG("Wrong index");
                DBGs(uhex2str(descridx));
            }
            break;
        case DEVICE_QUALIFIER_DESCRIPTOR:
            DBG("DEVICE_QUALIFIER_DESCRIPTOR");
            wr0(USB_DeviceQualifierDescriptor, sizeof(USB_DeviceQualifierDescriptor), pack->wLength);
            break;
       /* case HID_REPORT_DESCRIPTOR:
            wr0(HID_ReportDescriptor, sizeof(HID_ReportDescriptor), pack->wLength);
            break;*/
        default:
            break;
    }
}
