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

#include "modbusrtu.h"
#include "strfunc.h"

/*
MODBUS PROTOCOL:
uint32_t translates as two uint16_t in big-endian format

MC_READ_COIL - coil values (by bits)
MC_READ_DISCRETE - discrete -//-
MC_READ_HOLDING_REG - read time/led/inch/outch
MC_READ_INPUT_REG - read ADC channel (regno is ADC ch number)
MC_WRITE_COIL - change single relay
MC_WRITE_REG - set/reset LED, restart MCU
MC_WRITE_MUL_COILS - change several relays
MC_WRITE_MUL_REGS - change Tms
*/

// holding registers list (comment - corresponding CANbus command)
typedef enum{
    MR_RESET,           // CMD_RESET
    MR_TIME,            // CMD_TIME
    MR_LED,             // CMD_LED
    MR_INCHANNELS,      // CMD_INCHNLS
    MR_OUTCHANNELS,     // CMD_OUTCHNLS
} modbus_registers;

void parse_modbus_request(modbus_request *r);
void parse_modbus_response(modbus_response *r);
