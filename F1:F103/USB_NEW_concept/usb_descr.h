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

// definition of parts common for USB_DeviceDescriptor & USB_DeviceQualifierDescriptor
// bcdUSB: 1.10
#define bcdUSB              0x0110
#define bDeviceSubClass     0x00
#define bDeviceProtocol     0x00
#define idVendor            0x0403
#define idProduct           0xc631
#define bcdDevice_Ver       0x0205
#define bNumConfigurations  1

// low/high for uint16_t
#define L16(x)              (x & 0xff)
#define H16(x)              (x >> 8)

// amount of interfaces and endpoints (except 0) used
#define bNumInterfaces      1
#define bTotNumEndpoints    1

// powered
#define BusPowered          (1<<7)
#define SelfPowered         (1<<6)
#define RemoteWakeup        (1<<5)

extern const uint8_t USB_DeviceDescriptor[];
extern const uint8_t USB_DeviceQualifierDescriptor[];
extern const uint8_t USB_ConfigDescriptor[];
