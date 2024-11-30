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

#include <stm32f1.h>
#include <string.h>

#include "usb_dev.h"

static void EP1_Handler(){
}

void set_configuration(uint16_t _U_ configuration){
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

