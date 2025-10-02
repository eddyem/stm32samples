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

// amount of interfaces and endpoints (except 0) used
#define bNumInterfaces      2
#define bTotNumEndpoints    3
#define bNumCsInterfaces    4

// powered
#define BusPowered          (1<<7)
#define SelfPowered         (1<<6)
#define RemoteWakeup        (1<<5)

// buffer sizes
// for USB FS EP0 buffers are from 8 to 64 bytes long
#define USB_EP0BUFSZ    64
#define USB_EP1BUFSZ    10
// Rx/Tx EPs
#define USB_RXBUFSZ     64
#define USB_TXBUFSZ     64

// string descriptors
enum{
    iLANGUAGE_DESCR,
    iMANUFACTURER_DESCR,
    iPRODUCT_DESCR,
    iSERIAL_DESCR,
    iINTERFACE_DESCR1,
    iDESCR_AMOUNT
};

void get_descriptor(config_pack_t *pack);
