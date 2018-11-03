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
#include <string.h> // memcpy
#include "usart.h"


#define EP0DATABUF_SIZE                     (64)
#define DEVICE_DESCRIPTOR_SIZE_BYTE         (18)
#define DEVICE_QALIFIER_SIZE_BYTE           (10)
#define STRING_LANG_DESCRIPTOR_SIZE_BYTE    (4)

static usb_LineCoding lineCoding = {115200, 0, 0, 8};
static config_pack_t setup_packet;
static uint8_t ep0databuf[EP0DATABUF_SIZE];
static uint8_t ep0dbuflen = 0;
uint8_t setlinecoding = 0;

usb_LineCoding getLineCoding(){return lineCoding;}

const uint8_t USB_DeviceDescriptor[] = {
        DEVICE_DESCRIPTOR_SIZE_BYTE,   // bLength
        0x01,   // bDescriptorType - USB_DEVICE_DESC_TYPE
        0x10,   // bcdUSB_L - 1.10
        0x01,   // bcdUSB_H
        0xff,   // bDeviceClass - Vendor Specific Class
        0x00,   // bDeviceSubClass
        0x00,   // bDeviceProtocol
        0x08,   // bMaxPacketSize
        0x86,   // idVendor_L ch340: VID=0x1a86, PID=0x7523
        0x1a,   // idVendor_H
        0x23,   // idProduct_L
        0x75,   // idProduct_H
        0x36,   // bcdDevice_Ver_L
        0x02,   // bcdDevice_Ver_H
        0x00,   // iManufacturer
        0x02,   // iProduct USB2.0-Serial
        0x00,   // iSerialNumber
        0x01    // bNumConfigurations
};

const uint8_t USB_DeviceQualifierDescriptor[] = {
        DEVICE_QALIFIER_SIZE_BYTE,   //bLength
        0x06,   // bDescriptorType
        0x10,   // bcdUSB_L
        0x01,   // bcdUSB_H
        0xff,   // bDeviceClass
        0x00,   // bDeviceSubClass
        0x00,   // bDeviceProtocol
        0x08,   // bMaxPacketSize0
        0x01,   // bNumConfigurations
        0x00    // Reserved
};

const uint8_t USB_ConfigDescriptor[] = {
        /*Configuration Descriptor*/
        0x09, /* bLength: Configuration Descriptor size */
        0x02, /* bDescriptorType: Configuration */
        39,   /* wTotalLength:no of returned bytes */
        0x00,
        0x01, /* bNumInterfaces: 1 interface */
        0x01, /* bConfigurationValue: Configuration value */
        0x00, /* iConfiguration: Index of string descriptor describing the configuration */
        0x80, /* bmAttributes - Bus powered */
        0x32, /* MaxPower 100 mA */

        /*---------------------------------------------------------------------------*/

        /*Interface Descriptor */
        0x09, /* bLength: Interface Descriptor size */
        0x04, /* bDescriptorType: Interface */
        0x00, /* bInterfaceNumber: Number of Interface */
        0x00, /* bAlternateSetting: Alternate setting */
        0x03, /* bNumEndpoints: 3 endpoints used */
        0xff, /* bInterfaceClass */
        0x01, /* bInterfaceSubClass */
        0x02, /* bInterfaceProtocol */
        0x00, /* iInterface: */
///////////////////////////////////////////////////
        /*Endpoint IN2 Descriptor*/
        0x07, /* bLength: Endpoint Descriptor size */
        0x05, /* bDescriptorType: Endpoint */
        0x82, /* bEndpointAddress IN2 */
        0x02, /* bmAttributes: Bulk */
        0x20, /* wMaxPacketSize: 64 */
        0x00,
        0x00, /* bInterval: ignore for Bulk transfer */
        /*Endpoint OUT2 Descriptor*/
        0x07, /* bLength: Endpoint Descriptor size */
        0x05, /* bDescriptorType: Endpoint */
        0x02, /* bEndpointAddress: OUT2 */
        0x02, /* bmAttributes: Bulk */
        0x20, /* wMaxPacketSize: */
        0x00,
        0x00, /* bInterval: ignore for Bulk transfer */
        /*Endpoint 1 Descriptor*/
        0x07, /* bLength: Endpoint Descriptor size */
        0x05, /* bDescriptorType: Endpoint */
        0x81, /* bEndpointAddress IN1 */
        0x03, /* bmAttributes: Interrupt */
        0x08, /* wMaxPacketSize LO: */
        0x00, /* wMaxPacketSize HI: */
        0x01, /* bInterval: */       
};

const uint8_t USB_StringLangDescriptor[] = {
        STRING_LANG_DESCRIPTOR_SIZE_BYTE,   // bLength
        0x03,   // bDescriptorType
        0x09,   // wLANGID_L
        0x04    // wLANGID_H
};

// these descriptors are not used in PL2303 emulator!
_USB_STRING_(USB_StringSerialDescriptor, L"0.01")
_USB_STRING_(USB_StringManufacturingDescriptor, L"Russia, SAO RAS")
_USB_STRING_(USB_StringProdDescriptor, L"TSYS01 sensors controller")

static usb_dev_t USB_Dev;
static ep_t endpoints[MAX_ENDPOINTS];

/*
 * default handlers
 */
// SET_LINE_CODING
void WEAK linecoding_handler(__attribute__((unused)) usb_LineCoding *lc){
    #ifdef EBUG
    SEND("Want baudrate: "); printu(lc->dwDTERate);
    SEND(", charFormat: "); printu(lc->bCharFormat);
    SEND(", parityType: "); printu(lc->bParityType);
    SEND(", dataBits: "); printu(lc->bDataBits);
    usart_putchar('\n');
    #endif
    setlinecoding = 1;
}

// SET_CONTROL_LINE_STATE
void WEAK clstate_handler(__attribute__((unused)) uint16_t val){
    #ifdef EBUG
    SEND("change state to ");
    printu(val);
    usart_putchar('\n');
    #endif
}

// SEND_BREAK
void WEAK break_handler(){
    MSG("Break\n");
}

// handler of vendor requests
void WEAK vendor_handler(config_pack_t *packet){
    if(packet->bmRequestType & 0x80){ // read
        uint8_t c = '?';
        EP_WriteIRQ(0, &c, 1);
    }else{ // write ZLP
        EP_WriteIRQ(0, (uint8_t *)0, 0);
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
 * @param ep - endpoint state
 * @return data written to EP0R
 */
uint16_t EP0_Handler(ep_t ep){
    uint16_t status = 0; // bus powered
    uint16_t epstatus = ep.status; // EP0R on input -> return this value after modifications
    static uint8_t configuration = 0; // reply for GET_CONFIGURATION (==1 if configured)
    void wr0(const uint8_t *buf, uint16_t size){
        if(setup_packet.wLength < size) size = setup_packet.wLength;
        EP_WriteIRQ(0, buf, size);
    }
#ifdef EBUG
    uint8_t _2wr = 0;
    #define WRITEDUMP(str)  do{MSG(str); _2wr = 1;}while(0)
#else
    #define WRITEDUMP(str)
#endif
    if ((ep.rx_flag) && (ep.setup_flag)){
        if (setup_packet.bmRequestType == 0x80){ // standard device request (device to host)
            switch(setup_packet.bRequest){
                case GET_DESCRIPTOR:
                    switch(setup_packet.wValue){
                        case DEVICE_DESCRIPTOR:
                            wr0(USB_DeviceDescriptor, sizeof(USB_DeviceDescriptor));
                        break;
                        case CONFIGURATION_DESCRIPTOR:
                            wr0(USB_ConfigDescriptor, sizeof(USB_ConfigDescriptor));
                        break;
                        case STRING_LANG_DESCRIPTOR:
                            wr0(USB_StringLangDescriptor, STRING_LANG_DESCRIPTOR_SIZE_BYTE);
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
                        case DEVICE_QALIFIER_DESCRIPTOR:
                            wr0(USB_DeviceQualifierDescriptor, DEVICE_QALIFIER_SIZE_BYTE);
                        break;
                        default:
                            WRITEDUMP("UNK_DES");
                        break;
                    }
                break;
                case GET_STATUS:
                    EP_WriteIRQ(0, (uint8_t *)&status, 2); // send status: Bus Powered
                break;
                case GET_CONFIGURATION:
                    WRITEDUMP("GET_CONFIGURATION");
                    EP_WriteIRQ(0, &configuration, 1);
                break;
                default:
                    WRITEDUMP("80:WR_REQ");
                break;
            }
            epstatus = SET_NAK_RX(epstatus);
            epstatus = SET_VALID_TX(epstatus);
        }else if(setup_packet.bmRequestType == 0x00){ // standard device request (host to device)
            switch(setup_packet.bRequest){
                case SET_ADDRESS:
                    // new address will be assigned later - after  acknowlegement or request to host
                    USB_Dev.USB_Addr = setup_packet.wValue;
                break;
                case SET_CONFIGURATION:
                    // Now device configured
                    USB_Dev.USB_Status = USB_CONFIGURE_STATE;
                    configuration = setup_packet.wValue;
                break;
                default:
                    WRITEDUMP("0:WR_REQ");
                break;
            }
            // send ZLP
            EP_WriteIRQ(0, (uint8_t *)0, 0);
            epstatus = SET_NAK_RX(epstatus);
            epstatus = SET_VALID_TX(epstatus);
        }else if(setup_packet.bmRequestType == 0x02){ // standard endpoint request (host to device)
            if (setup_packet.bRequest == CLEAR_FEATURE){
                // send ZLP
                EP_WriteIRQ(0, (uint8_t *)0, 0);
                epstatus = SET_NAK_RX(epstatus);
                epstatus = SET_VALID_TX(epstatus);
            }else{
                WRITEDUMP("02:WR_REQ");
            }
        }else if((setup_packet.bmRequestType & VENDOR_MASK_REQUEST) == VENDOR_MASK_REQUEST){ // vendor request
            vendor_handler(&setup_packet);
            WRITEDUMP("VENDOR");
            epstatus = SET_NAK_RX(epstatus);
            epstatus = SET_VALID_TX(epstatus);
        }else if((setup_packet.bmRequestType & 0x7f) == CONTROL_REQUEST_TYPE){ // control request
            switch(setup_packet.bRequest){
                case GET_LINE_CODING:
                    EP_WriteIRQ(0, (uint8_t*)&lineCoding, sizeof(lineCoding));
                break;
                case SET_LINE_CODING:
                break;
                case SET_CONTROL_LINE_STATE:
                    clstate_handler(setup_packet.wValue);
                break;
                case SEND_BREAK:
                    break_handler();
                break;
                default:
                    WRITEDUMP("undef control req");
            }
            if((setup_packet.bmRequestType & 0x80) == 0) EP_WriteIRQ(0, (uint8_t *)0, 0); // write acknowledgement
            epstatus = SET_VALID_RX(epstatus);
            epstatus = SET_VALID_TX(epstatus);
        }
    }else if (ep.rx_flag){ // got data over EP0 or host acknowlegement
        if(ep.rx_cnt){
            if(setup_packet.bRequest == SET_LINE_CODING){
                WRITEDUMP("SET_LINE_CODING");
                linecoding_handler((usb_LineCoding*)ep0databuf);
            }
            EP_WriteIRQ(0, (uint8_t *)0, 0);
        }
        // Close transaction
        epstatus = CLEAR_DTOG_RX(epstatus);
        epstatus = CLEAR_DTOG_TX(epstatus);
        // wait for new data from host
        epstatus = SET_VALID_RX(epstatus);
        epstatus = SET_STALL_TX(epstatus);
    } else if (ep.tx_flag){ // package transmitted
        // now we can change address after enumeration
        if ((USB->DADDR & USB_DADDR_ADD) != USB_Dev.USB_Addr){
            USB->DADDR = USB_DADDR_EF | USB_Dev.USB_Addr;
            // change state to ADRESSED
            USB_Dev.USB_Status = USB_ADRESSED_STATE;
        }
        // end of transaction
        epstatus = CLEAR_DTOG_RX(epstatus);
        epstatus = CLEAR_DTOG_TX(epstatus);
        epstatus = SET_VALID_RX(epstatus);
        epstatus = SET_VALID_TX(epstatus);
    }
#ifdef EBUG
    if(_2wr){
        usart_putchar(' ');
        if (ep.rx_flag) usart_putchar('r');
        else usart_putchar('t');
        printu(setup_packet.wLength);
        if(ep.setup_flag) usart_putchar('s');
        usart_putchar(' ');
        usart_putchar('I');
        printu(setup_packet.wIndex);
        usart_putchar('V');
        printu(setup_packet.wValue);
        usart_putchar('R');
        printu(setup_packet.bRequest);
        usart_putchar('T');
        printu(setup_packet.bmRequestType);
        usart_putchar(' ');
        usart_putchar('0' + ep0dbuflen);
        usart_putchar(' ');
        hexdump(ep0databuf, ep0dbuflen);
        usart_putchar('\n');
    }
#endif
    return epstatus;
}

/**
 * Endpoint initialisation, size of input buffer fixed to 64 bytes
 * @param number - EP num (0...7)
 * @param type - EP type (EP_TYPE_BULK, EP_TYPE_CONTROL, EP_TYPE_ISO, EP_TYPE_INTERRUPT)
 * @param addr_tx - transmission buffer address @ USB/CAN buffer
 * @param addr_rx - reception buffer address @ USB/CAN buffer
 * @param uint16_t (*func)(ep_t *ep) - EP handler function
 */
void EP_Init(uint8_t number, uint8_t type, uint16_t addr_tx, uint16_t addr_rx, uint16_t (*func)(ep_t ep)){
    USB->EPnR[number] = (type << 9) | (number & USB_EPnR_EA);
    USB->EPnR[number] ^= USB_EPnR_STAT_RX | USB_EPnR_STAT_TX_1;
    USB_BTABLE->EP[number].USB_ADDR_TX = addr_tx;
    USB_BTABLE->EP[number].USB_COUNT_TX = 0;
    USB_BTABLE->EP[number].USB_ADDR_RX = addr_rx;
    USB_BTABLE->EP[number].USB_COUNT_RX = 0x8400; // buffer size (64 bytes): Table127 of RM: BL_SIZE=1, NUM_BLOCK=1
    endpoints[number].func = func;
    endpoints[number].tx_buf = (uint16_t *)(USB_BTABLE_BASE + addr_tx);
    endpoints[number].rx_buf = (uint8_t *)(USB_BTABLE_BASE + addr_rx);
}

// standard IRQ handler
void usb_isr(){
    uint8_t n;
    if (USB->ISTR & USB_ISTR_RESET){
        // Reinit registers
        USB->CNTR = USB_CNTR_RESETM | USB_CNTR_CTRM;
        USB->ISTR = 0;
        // Endpoint 0 - CONTROL
        EP_Init(0, EP_TYPE_CONTROL, 64, 128, EP0_Handler);
        // clear address, leave only enable bit
        USB->DADDR = USB_DADDR_EF;
        // state is default - wait for enumeration
        USB_Dev.USB_Status = USB_DEFAULT_STATE;
    }
    while(USB->ISTR & USB_ISTR_CTR){
        // EP number
        n = USB->ISTR & USB_ISTR_EPID;
        // copy status register
        uint16_t epstatus = USB->EPnR[n];
        // Calculate flags
        endpoints[n].rx_flag = (epstatus & USB_EPnR_CTR_RX) ? 1 : 0;
        endpoints[n].setup_flag = (epstatus & USB_EPnR_SETUP) ? 1 : 0;
        endpoints[n].tx_flag = (epstatus & USB_EPnR_CTR_TX) ? 1 : 0;
        // copy received bytes amount
        endpoints[n].rx_cnt = USB_BTABLE->EP[n].USB_COUNT_RX;
        // check direction
        if(USB->ISTR & USB_ISTR_DIR){ // OUT interrupt - receive data, CTR_RX==1 (if CTR_TX == 1 - two pending transactions: receive following by transmit)
            if(n == 0){ // control endpoint
                if(epstatus & USB_EPnR_SETUP){ // setup packet -> copy data to conf_pack
                    memcpy(&setup_packet, endpoints[0].rx_buf, sizeof(setup_packet));
                    ep0dbuflen = 0;
                    // interrupt handler will be called later
                }else if(epstatus & USB_EPnR_CTR_RX){ // data packet -> push received data to ep0databuf
                    ep0dbuflen = endpoints[0].rx_cnt;
                    memcpy(ep0databuf, endpoints[0].rx_buf, ep0dbuflen);
                }
            }
        }else{ // IN interrupt - transmit data, only CTR_TX == 1
            // enumeration end could be here (if EP0)
        }
        // prepare status field for EP handler
        endpoints[n].status = epstatus;
        // call EP handler (even if it will change EPnR, it should return new status)
        epstatus = endpoints[n].func(endpoints[n]);
        // keep DTOG state
        epstatus = KEEP_DTOG_TX(epstatus);
        epstatus = KEEP_DTOG_RX(epstatus);
        // clear all RX/TX flags
        epstatus = CLEAR_CTR_RX(epstatus);
        epstatus = CLEAR_CTR_TX(epstatus);
        // refresh EPnR
        USB->EPnR[n] = epstatus;
        USB_BTABLE->EP[n].USB_COUNT_RX = 0x8400;
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
    uint16_t N2 = (size + 1) >> 1;
    // the buffer is 16-bit, so we should copy data as it would be uint16_t
    uint16_t *buf16 = (uint16_t *)buf;
    for (i = 0; i < N2; i++){
        endpoints[number].tx_buf[i] = buf16[i];
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
    uint16_t status = USB->EPnR[number];
    EP_WriteIRQ(number, buf, size);
    status = SET_NAK_RX(status);
    status = SET_VALID_TX(status);
    status = KEEP_DTOG_TX(status);
    status = KEEP_DTOG_RX(status);
    USB->EPnR[number] = status;
}

/*
 * Copy data from EP buffer into user buffer area
 * @param *buf - user array for data
 * @return amount of data read
 */
int EP_Read(uint8_t number, uint8_t *buf){
    int i = endpoints[number].rx_cnt;
    if(i) memcpy(buf, endpoints[number].rx_buf, i);
    return i;
}

// USB status
uint8_t USB_GetState(){
    return USB_Dev.USB_Status;
}
