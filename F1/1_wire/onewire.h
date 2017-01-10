/*
 * onewire.h
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
 */

#pragma once
#ifndef __ONEWIRE_H__
#define __ONEWIRE_H__

#include "main.h"
#include "hardware_ini.h"

// 20 bytes x 8bits
#define TIM2_DMABUFF_SIZE 160

// freq = 1MHz
// ARR values: 1000 for reset, 100 for data in/out
// CCR4 values: 500 for reset, 60 for sending 0 or reading, <15 for sending 1
// CCR3 values: >550 if there's devices on line (on reset), >12 (typ.15) - read 0, < 12 (typ.1) - read 1
#define RESET_LEN         ((uint16_t)1000)
#define BIT_LEN           ((uint16_t)100)
#define RESET_P           ((uint16_t)500)
#define BIT_ONE_P         ((uint16_t)10)
#define BIT_ZERO_P        ((uint16_t)60)
#define BIT_READ_P        ((uint16_t)5)
#define RESET_BARRIER     ((uint16_t)550)
#define ONE_ZERO_BARRIER  ((uint16_t)10)

#define ERR_TEMP_VAL  ((int32_t)200000)

typedef struct{
	uint8_t bytes[8];
} OW_ID;

extern OW_ID id_array[];

#define OW_MAX_NUM 8

void init_ow_dmatimer();
void run_dmatimer();
extern uint8_t ow_done;
#define OW_READY()  (ow_done)
void ow_dma_on();
uint8_t OW_add_byte(uint8_t ow_byte);
uint8_t OW_add_read_seq(uint8_t Nbytes);
void read_from_OWbuf(uint8_t start_idx, uint8_t N, uint8_t *outbuf);
void ow_reset();
uint8_t OW_get_reset_status();

extern int tum2buff_ctr;
#define OW_reset_buffer()   do{tum2buff_ctr = 0;}while(0)

extern uint8_t ow_data_ready;
extern uint8_t ow_measurements_done;
#define OW_DATA_READY()       (ow_data_ready)
#define OW_CLEAR_READY_FLAG() do{ow_data_ready = 0;}while(0)
#define OW_MEASUREMENTS_DONE()  (ow_measurements_done)
#define OW_CLEAR_DONE_FLAG()  do{ow_measurements_done = 0;}while(0)

void OW_process();
void OW_fill_next_ID();
void OW_send_read_seq();
uint8_t OW_Send(uint8_t sendReset, uint8_t *command, uint8_t cLen);

extern int32_t temperature;
extern int8_t Ncur;
void OW_read_next_temp();
#define OW_current_temp()  (temperature)
#define OW_current_num()   (Ncur)

/*
 * thermometer commands
 * send them with bus reset!
 */
// find devices
#define OW_SEARCH_ROM       (0xf0)
// read device (when it is alone on the bus)
#define OW_READ_ROM         (0x33)
// send device ID (after this command - 8 bytes of ID)
#define OW_MATCH_ROM        (0x55)
// broadcast command
#define OW_SKIP_ROM         (0xcc)
// find devices with critical conditions
#define OW_ALARM_SEARCH     (0xec)
/*
 * thermometer functions
 * send them without bus reset!
 */
// start themperature reading
#define OW_CONVERT_T         (0x44)
// write critical temperature to device's RAM
#define OW_SCRATCHPAD        (0x4e)
// read whole device flash
#define OW_READ_SCRATCHPAD   (0xbe)
// copy critical themperature from device's RAM to its EEPROM
#define OW_COPY_SCRATCHPAD   (0x48)
// copy critical themperature from EEPROM to RAM (when power on this operation runs automatically)
#define OW_RECALL_E2         (0xb8)
// check whether there is devices wich power up from bus
#define OW_READ_POWER_SUPPLY (0xb4)


/*
 * thermometer identificator is: 8bits CRC, 48bits serial, 8bits device code (10h)
 * Critical temperatures is T_H and T_L
 * T_L is lowest allowed temperature
 * T_H is highest -//-
 * format T_H and T_L: 1bit sigh + 7bits of data
 */


/*
 * RAM register:
 * 0 - themperature: 1 ADU == 0.5 degrC
 * 1 - sign (0 - T>0 degrC ==> T=byte0; 1 - T<0 degrC ==> T=byte0-0xff+1)
 * 2 - T_H
 * 3 - T_L
 * 4 - 0xff (reserved)
 * 5 - 0xff (reserved)
 * 6 - COUNT_REMAIN (0x0c)
 * 7 - COUNT PER DEGR (0x10)
 * 8 - CRC
 */


#endif // __ONEWIRE_H__
