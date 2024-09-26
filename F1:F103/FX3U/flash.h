/*
 * This file is part of the fx3u project.
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

#include <stm32f1.h>

#define FLASH_SIZE_REG      ((uint32_t)0x1FFFF7E0)
#define FLASH_SIZE          *((uint16_t*)FLASH_SIZE_REG)

// maximal bit NUMBER (0 etc) of flags
#define MAX_FLAG_BITNO      (3)
typedef union{
    uint32_t u32;
    struct{
        uint32_t sw_send_esw_can : 1;       // 0 - if ESW state changes, send message over CAN with CANID_IN
        uint32_t sw_send_relay_can : 1;     // 1 - switching ESW state will send CMD_RELAY command with CANID_OUT
        uint32_t sw_send_relay_inv : 1;     // 2 - sw_send should be inverted related to current inputs value
        uint32_t sw_send_relay_modbus : 1;  // 3 - 1 for modbus
    };
} confflags_t;

// ALL sw_send flags mask (without relay_inv)
#define SW_SEND_MASK    (0x0B)

/*
 * struct to save user configurations
 */
typedef struct __attribute__((aligned(4))){
    uint16_t userconf_sz;       // "magick number"
    uint16_t CANIDin;           // CAN bus device ID for input commands
    uint16_t CANIDout;          // -//- for output signals
    uint16_t reserved;          // added for 32-bit align
    uint32_t bouncetime;        // anti-bounce timeout (ms)
    uint32_t usartspeed;        // RS-232 speed
    uint32_t CANspeed;          // CAN bus speed
    confflags_t flags;          // different flags
    uint32_t modbusID;          // MODBUS-RTU ID (0 for master)
    uint32_t modbusIDout;       // MODBUS-RTU slave ID to send relay command when IN changes
    uint32_t modbusspeed;       // Speed of modbus interface
} user_conf;

extern user_conf the_conf;
extern const uint32_t FLASH_blocksize;
extern const user_conf *Flash_Data;
extern int currentconfidx;

void flashstorage_init();
int store_userconf();
int erase_storage(int npage);
