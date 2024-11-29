/*
 * This file is part of the I2Ctiny project.
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

#include <stm32f1.h>
#include <string.h>

#include "i2c.h"
#include "i2ctiny.h"
#include "usbdev.h"

static const uint32_t func = I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_NOSTART;
static uint8_t status = STATUS_IDLE;

// interrupt IN handler - never used
static void EP1_Handler(){
    uint16_t epstatus = KEEP_DTOG_STAT(USB->EPnR[1]);
    if(RX_FLAG(epstatus)) epstatus = (epstatus & ~USB_EPnR_STAT_TX) ^ USB_EPnR_STAT_RX; // set valid RX
    else epstatus = epstatus & ~(USB_EPnR_STAT_TX|USB_EPnR_STAT_RX);
    // clear CTR
    epstatus = (epstatus & ~(USB_EPnR_CTR_RX|USB_EPnR_CTR_TX));
    USB->EPnR[1] = epstatus;
}

void set_configuration(uint16_t _U_ configuration){
    EP_Init(1, EP_TYPE_INTERRUPT, USB_EP1BUFSZ, 0, EP1_Handler);
}

static void usb_i2c_io(config_pack_t *req, uint8_t *buf, size_t *len){
    // static uint8_t iobuf[256] = "1234567890abcdefghijclmnop";
    uint8_t cmd = req->bRequest;
    uint8_t size = req->wLength;
    uint8_t is_read = req->wValue & I2C_M_RD;
    i2c_status stat = I2C_NACK;
    if(cmd & CMD_I2C_BEGIN){
        if(I2C_OK != (stat = i2c_start())) goto eot;
        if(I2C_OK != (stat = i2c_sendaddr((uint8_t)req->wIndex, (is_read) ? size : 0))) goto eot;
    }
    for(int i = 0; i < size; ++i){
        if(is_read) stat = i2c_readbyte(buf + i);
        else stat = i2c_sendbyte(buf[i]);
        if(I2C_OK != stat) goto eot;
    }
    if(cmd & CMD_I2C_END) stat = i2c_stop();
eot:
    if(stat == I2C_OK){
        status = STATUS_ADDRESS_ACK;
        *len = (is_read) ? size : 0;
    }else{
        i2c_setup();
        *len = 0;
        status = STATUS_ADDRESS_NACK;
    }
}

void usb_class_request(config_pack_t *req, uint8_t *data, unsigned int datalen){
    uint8_t buf[USB_EP0BUFSZ];
    size_t len = 0;
    //uint8_t recipient = REQUEST_RECIPIENT(req->bmRequestType);
    if((req->bmRequestType & 0x80) == 0){ // OUT - setters
        switch(req->bRequest){
            case CMD_I2C_IO:
            case CMD_I2C_IO | CMD_I2C_BEGIN:
            case CMD_I2C_IO | CMD_I2C_END:
            case CMD_I2C_IO | CMD_I2C_BEGIN | CMD_I2C_END: // write
                if(req->wValue & I2C_M_RD) break; // OUT - only write
                if(!data) break; // wait for data
                len = datalen;
                usb_i2c_io(req, data, &len);
            break;
            default:
            break;
        }
        EP_WriteIRQ(0, 0, 0);
        return;
    }
    switch(req->bRequest){
        case CMD_ECHO:
            memcpy(buf, &req->wValue, sizeof(req->wValue));
            len = sizeof(req->wValue);
        break;
        case CMD_GET_FUNC:
            /* Report our capabilities */
            bzero(buf, req->wLength);
            memcpy(buf, &func, sizeof(func));
            len = req->wLength;
        break;
        case CMD_I2C_IO:
        case CMD_I2C_IO | CMD_I2C_BEGIN:
        case CMD_I2C_IO | CMD_I2C_END:
        case CMD_I2C_IO | CMD_I2C_BEGIN | CMD_I2C_END: // read
            if(req->wValue & I2C_M_RD){ // IN - only read
                len = req->wLength;
                usb_i2c_io(req, buf, &len);
            }
        break;
        case CMD_GET_STATUS:
            memcpy(buf, &status, sizeof(status));
            len = sizeof(status);
        break;
        default:
        break;
    }
    EP_WriteIRQ(0, buf, len); // write ZLP if nothing received
}

void usb_vendor_request(config_pack_t *req, uint8_t *data, unsigned int datalen) __attribute__ ((alias ("usb_class_request")));

#if 0
// handler of vendor requests
void usb_vendor_request(config_pack_t *req){
    uint8_t buf[USB_EP0BUFSZ];
    size_t len = 0;
    if((req->bmRequestType & 0x80) == 0){ // OUT
        EP_WriteIRQ(0, 0, 0);
        return;
    }
    switch(req->bRequest){
        case CMD_ECHO:
            memcpy(buf, &req->wValue, sizeof(req->wValue));
            len = sizeof(req->wValue);
            break;
        case CMD_GET_FUNC:
            /* Report our capabilities */
            memcpy(buf, &func, sizeof(func));
            len = sizeof(func);
            break;
/*        case CMD_I2C_IO:
        case CMD_I2C_IO | CMD_I2C_BEGIN:
        case CMD_I2C_IO | CMD_I2C_END:
        case CMD_I2C_IO | CMD_I2C_BEGIN | CMD_I2C_END:
            if(req->wValue & I2C_M_RD) bptr = buf;
            else{
                bptr = data;
                len = datalen;
            }
            usb_i2c_io(req, bptr, &len);
            break;*/
        case CMD_GET_STATUS:
            memcpy(buf, &status, sizeof(status));
            len = sizeof(status);
            break;
        default:
            break;
    }
    EP_WriteIRQ(0, buf, len); // write ZLP if nothing received
#if 0
    buf[0] = 'D';
    buf[1] = 'V';
    switch(recipient){
        case REQ_RECIPIENT_DEVICE:
            buf[2] = 'D';
            //return;
        break;
        case REQ_RECIPIENT_INTERFACE:
            buf[2] = 'I';
        break;
        case REQ_RECIPIENT_ENDPOINT:
            buf[2] = 'E';
        break;
        default:
            buf[2] = 'O';
        return;
    }
    EP_WriteIRQ(0, buf, 3);
    return;
    switch(recipient){
        case REQ_RECIPIENT_DEVICE:
        break;
        default:
        return;
            /*
        case REQ_RECIPIENT_INTERFACE:
        break;
        case REQ_RECIPIENT_ENDPOINT:
        break;
        default:
        break;*/
    }
#endif
}
#endif
