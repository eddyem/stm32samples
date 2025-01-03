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

#include "usart.h"
#include "usb_descr.h"

/*
#undef DBG
#define DBG(x)
#undef DBGs
#define DBGs(x)
*/

// low/high for uint16_t
#define L16(x)              (x & 0xff)
#define H16(x)              (x >> 8)

static const uint8_t USB_DeviceDescriptor[] = {
    USB_DT_DEVICE_SIZE, // bLength
    USB_DT_DEVICE, // bDescriptorType
    L16(bcdUSB), // bcdUSB_L
    H16(bcdUSB), // bcdUSB_H
    USB_CLASS_PER_INTERFACE, // bDeviceClass
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

static const uint8_t HID_ReportDescriptor[] = {
    0x05, 0x01, /* Usage Page (Generic Desktop)             */
    0x09, 0x02, /* Usage (Mouse)                            */
    0xA1, 0x01, /* Collection (Application)                 */
    0x09, 0x01, /*  Usage (Pointer)                         */
    0xA1, 0x00, /*  Collection (Physical)                   */
    0x85, 0x01,  /*   Report ID  */
    0x05, 0x09, /*      Usage Page (Buttons)                */
    0x19, 0x01, /*      Usage Minimum (01)                  */
    0x29, 0x03, /*      Usage Maximum (03)                  */
    0x15, 0x00, /*      Logical Minimum (0)                 */
    0x25, 0x01, /*      Logical Maximum (0)                 */
    0x95, 0x03, /*      Report Count (3)                    */
    0x75, 0x01, /*      Report Size (1)                     */
    0x81, 0x02, /*      Input (Data, Variable, Absolute)    */
    0x95, 0x01, /*      Report Count (1)                    */
    0x75, 0x05, /*      Report Size (5)                     */
    0x81, 0x01, /*      Input (Constant)    ;5 bit padding  */
    0x05, 0x01, /*      Usage Page (Generic Desktop)        */
    0x09, 0x30, /*      Usage (X)                           */
    0x09, 0x31, /*      Usage (Y)                           */
    0x15, 0x81, /*      Logical Minimum (-127)              */
    0x25, 0x7F, /*      Logical Maximum (127)               */
    0x75, 0x08, /*      Report Size (8)                     */
    0x95, 0x02, /*      Report Count (2)                    */
    0x81, 0x06, /*      Input (Data, Variable, Relative)    */
    0xC0, 0xC0,/* End Collection,End Collection            */
    //
    0x09, 0x06, /*		Usage (Keyboard)                    */
    0xA1, 0x01, /*		Collection (Application)            */
    0x85, 0x02,  /*   Report ID  */
    0x05, 0x07, /*  	Usage (Key codes)                   */
    0x19, 0xE0, /*      Usage Minimum (224)                 */
    0x29, 0xE7, /*      Usage Maximum (231)                 */
    0x15, 0x00, /*      Logical Minimum (0)                 */
    0x25, 0x01, /*      Logical Maximum (1)                 */
    0x75, 0x01, /*      Report Size (1)                     */
    0x95, 0x08, /*      Report Count (8)                    */
    0x81, 0x02, /*      Input (Data, Variable, Absolute)    */
    0x95, 0x01, /*      Report Count (1)                    */
    0x75, 0x08, /*      Report Size (8)                     */
    0x81, 0x01, /*      Input (Constant)    ;5 bit padding  */
    0x95, 0x05, /*      Report Count (5)                    */
    0x75, 0x01, /*      Report Size (1)                     */
    0x05, 0x08, /*      Usage Page (Page# for LEDs)         */
    0x19, 0x01, /*      Usage Minimum (01)                  */
    0x29, 0x05, /*      Usage Maximum (05)                  */
    0x91, 0x02, /*      Output (Data, Variable, Absolute)   */
    0x95, 0x01, /*      Report Count (1)                    */
    0x75, 0x03, /*      Report Size (3)                     */
    0x91, 0x01, /*      Output (Constant)                   */
    0x95, 0x06, /*      Report Count (1)                    */
    0x75, 0x08, /*      Report Size (3)                     */
    0x15, 0x00, /*      Logical Minimum (0)                 */
    0x25, 0x65, /*      Logical Maximum (101)               */
    0x05, 0x07, /*  	Usage (Key codes)                   */
    0x19, 0x00, /*      Usage Minimum (00)                  */
    0x29, 0x65, /*      Usage Maximum (101)                 */
    0x81, 0x00, /*      Input (Data, Array)                 */
    0xC0        /* 		End Collection,End Collection       */
};

#define wTotalLength  (USB_DT_CONFIG_SIZE + (bNumInterfaces * USB_DT_INTERFACE_SIZE) + (bTotNumEndpoints * USB_DT_ENDPOINT_SIZE) + USB_DT_HID_SIZE)

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
    //---------------------------------------------------------------------------
    // Interface Descriptor
    USB_DT_INTERFACE_SIZE, // bLength: Interface Descriptor size
    USB_DT_INTERFACE, // bDescriptorType: Interface
    0, // bInterfaceNumber: Number of Interface
    0, // bAlternateSetting: Alternate setting
    bTotNumEndpoints, // bNumEndpoints
    USB_CLASS_HID, // bInterfaceClass
    1, // bInterfaceSubClass: boot
    1, // bInterfaceProtocol: keyboard
    0, // iInterface:
    //---------------------------------------------------------------------------
    // HID device descriptor
    USB_DT_HID_SIZE, // bLength
    USB_DT_HID, // bDescriptorType: HID
    L16(bcdUSB), // bcdUSB_L
    H16(bcdUSB), // bcdUSB_H
    0, // bCountryCode: Not supported
    1, // bNumDescriptors: 1
    USB_DT_REPORT, // bDescriptorType: Report
    L16(sizeof(HID_ReportDescriptor)), // wDescriptorLength
    H16(sizeof(HID_ReportDescriptor)),
    // Endpoint 1 Descriptor
    USB_DT_ENDPOINT_SIZE, // bLength: Endpoint Descriptor size
    USB_DT_ENDPOINT, // bDescriptorType: Endpoint
    0x81, // bEndpointAddress IN1
    USB_BM_ATTR_INTERRUPT, // bmAttributes: Interrupt
    L16(USB_EP1BUFSZ), // wMaxPacketSize LO
    H16(USB_EP1BUFSZ), // wMaxPacketSize HI
    0x01, // bInterval: 1ms
};

_USB_LANG_ID_(LD, LANG_US);
_USB_STRING_(SD, u"0.0.1");
_USB_STRING_(MD, u"eddy@sao.ru");
_USB_STRING_(PD, u"USB HID mouse+keyboard");
//_USB_STRING_(ID, u"mouse_keyboard");

static const void* const StringDescriptor[iDESCR_AMOUNT] = {
    [iLANGUAGE_DESCR] = &LD,
    [iMANUFACTURER_DESCR] = &MD,
    [iPRODUCT_DESCR] = &PD,
    [iSERIAL_DESCR] = &SD,
//    [iINTERFACE_DESCR1] = &ID
};

void wr0(const uint8_t *buf, uint16_t size, uint16_t askedsize){
    if(askedsize < size) size = askedsize; // shortened request
    if(size < USB_EP0BUFSZ){
        //DBG("short wr0");
        EP_WriteIRQ(0, buf, size);
        return;
    }
    //DBG("long wr0");
    while(size){
        //DBG("write");
        //DBGs(uhex2str(size));
        uint16_t l = size;
        if(l > USB_EP0BUFSZ) l = USB_EP0BUFSZ;
        //DBGs(uhex2str(l));
        EP_WriteIRQ(0, buf, l);
        buf += l;
        size -= l;
        uint8_t needzlp = (l == USB_EP0BUFSZ) ? 1 : 0;
        if(size || needzlp){ // send next data buffer
            uint16_t epstatus = KEEP_DTOG(USB->EPnR[0]);
            // keep DTOGs, clear CTR_TX/CTR_RX, set TX VALID, leave stat_Rx
            USB->EPnR[0] = (epstatus & ~(USB_EPnR_CTR_RX|USB_EPnR_CTR_TX|USB_EPnR_STAT_RX))
                           ^ USB_EPnR_STAT_TX;
            //USB->EPnR[0] = (epstatus & ~(USB_EPnR_CTR_TX|USB_EPnR_STAT_RX))
            //                               ^ USB_EPnR_STAT_TX;
            uint32_t ctr = 100000;
            while(--ctr && (USB->ISTR & USB_ISTR_CTR) == 0){IWDG->KR = IWDG_REFRESH;};
            //DBGs(uhex2str(USB->ISTR));
            //DBGs(uhex2str(USB->EPnR[0]));
            if((USB->ISTR & USB_ISTR_CTR) == 0){
                DBG("No CTR!");
                return;
            }
            if(needzlp) EP_WriteIRQ(0, NULL, 0);
        }
    }
}

void get_descriptor(config_pack_t *pack){
    uint8_t descrtype = pack->wValue >> 8,
        descridx = pack->wValue & 0xff;
    DBG("DESCR:");
    DBGs(uhex2str(descrtype));
    DBGs(uhex2str(descridx));
    DBGs(uhex2str(pack->wLength));
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
            if(descridx < iDESCR_AMOUNT) wr0((const uint8_t *)StringDescriptor[descridx], *((uint8_t*)StringDescriptor[descridx]), pack->wLength);
            else EP_WriteIRQ(0, NULL, 0);
            break;
        case DEVICE_QUALIFIER_DESCRIPTOR:
            DBG("DEVICE_QUALIFIER_DESCRIPTOR");
            wr0(USB_DeviceQualifierDescriptor, sizeof(USB_DeviceQualifierDescriptor), pack->wLength);
            break;
        case HID_REPORT_DESCRIPTOR:
            DBG("HID_REPORT_DESCRIPTOR");
            wr0(HID_ReportDescriptor, sizeof(HID_ReportDescriptor), pack->wLength);
            break;
        default:
            DBG("Wrong");
            DBGs(uhex2str(descrtype));
            break;
    }
}
