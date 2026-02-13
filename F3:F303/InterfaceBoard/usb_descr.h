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

#include "usb_lib.h"

// definition of parts common for USB_DeviceDescriptor & USB_DeviceQualifierDescriptor
// bcdUSB: 1.10
#define bcdUSB              0x0110
// Class - Misc (EF), subclass - common (2), protocol - interface association descr (1)
#define bDeviceSubClass     0x02
#define bDeviceProtocol     0x01
#define idVendor            0x0483
#define idProduct           0x5740
#define bcdDevice_Ver       0x0200
#define bNumConfigurations  1

// amount of interfaces
#define InterfacesAmount    7
// index of interface (ep number minus one): IF1..IF7
#define ISerial0            0
#define ISerial1            1
#define ISerial2            2
#define ISerial3            3
#define ISerial4            4
#define ICAN                5
#define ISPI                6
#define ICFG                6
// EP number of interface
#define EPNO(i) (i + 1)

// amount of interfaces (including virtual) except 0
#define bNumInterfaces      (2*InterfacesAmount)
// amount of endpoints used
#define bTotNumEndpoints    (1+InterfacesAmount)

// powered
#define BusPowered          (1<<7)
#define SelfPowered         (1<<6)
#define RemoteWakeup        (1<<5)

// buffer sizes
// for USB FS EP0 buffers are from 8 to 64 bytes long
#define USB_EP0BUFSZ    16
// virtual
#define USB_EP1BUFSZ    10
// Rx/Tx EPs (USB_BTABLE_SIZE-64-2*USB_EP0BUFSZ)/(2*InterfacesAmount) rounded to 8
// ([btable size] - [registers] - [EP0 buffers])/([interfaces buffers (Rx/Tx)])/8      (for rounding by 8 later)
#define _RTBUFSZ8       (((USB_BTABLE_SIZE)/(ACCESSZ) - (LASTADDR_DEFAULT) - (2 * (USB_EP0BUFSZ)))/(16 * (InterfacesAmount)))
// 32 bytes; so we have free 86 bytes which can't be used
#define USB_RXBUFSZ     (8 * (_RTBUFSZ8))
#define USB_TXBUFSZ     (8 * (_RTBUFSZ8))
//#define USB_RXBUFSZ 8
//#define USB_TXBUFSZ 8

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

void get_descriptor(config_pack_t *pack);
void setup_interfaces();
