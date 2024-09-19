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
#include <stdint.h>

// input and output buffers size
#define MODBUSBUFSZI  (64)
#define MODBUSBUFSZO  (64)

#define MODBUS_MASTER_ID    (0)
#define MODBUS_MAX_ID       (247)

// some exceptions
typedef enum{
    ME_ILLEGAL_FUNCION = 1, // The function code received in the request is not an authorized action for the slave.
    ME_ILLEGAL_ADDRESS = 2, // The data address received by the slave is not an authorized address for the slave.
    ME_ILLEGAL_VALUE = 3,   // The value in the request data field is not an authorized value for the slave.
    ME_SLAVE_FAILURE = 4,   // The slave fails to perform a requested action because of an unrecoverable error.
    ME_ACK = 5,             // The slave accepts the request but needs a long time to process it.
    ME_SLAVE_BUSY = 6,      // The slave is busy processing another command.
    ME_NACK = 7,            // The slave cannot perform the programming request sent by the master.
    ME_PARITY_ERROR = 8,    // Memory parity error: slave is almost dead.
} modbus_exceptions;

// common function codes; "by bits" means that output data length = (requested+15)/16
typedef enum{
    MC_READ_COIL = 1,           // read output[s] state (by bits)
    MC_READ_DISCRETE = 2,       // read input[s] state (by bits!)
    MC_READ_HOLDING_REG = 3,
    MC_READ_INPUT_REG = 4,
    MC_WRITE_COIL = 5,
    MC_WRITE_REG = 6,
    MC_WRITE_MUL_COILS = 0xF,
    MC_WRITE_MUL_REGS = 0x10,
} modbus_fcode;

// modbus master request (without CRC)
typedef struct{
    uint8_t ID;     // slave ID
    uint8_t Fcode;  // functional code
    uint16_t startreg; // started register or single register address
    uint16_t regno; // number of registers or data to write
    uint8_t datalen;// data for 0xf/0x10
    uint8_t *data;
} modbus_request;

// responce->Fcode & RESPONCE_ERRMARK -> error packet
#define MODBUS_RESPONSE_ERRMARK    (0x80)

// modbus slave responce (without CRC)
typedef struct{
    uint8_t ID;         // slave ID
    uint8_t Fcode;      // functional code
    uint8_t datalen;    // length of data in BYTES! (or exception code)
    uint8_t *data;      // data (or NULL for error packet); BE CAREFUL: all data is big-endian!
} modbus_response;

void modbus_setup(uint32_t speed);
int modbus_receive(uint8_t **packet);
int modbus_get_request(modbus_request* r);
int modbus_get_response(modbus_response* r);
int modbus_send(uint8_t *data, int l);
int modbus_send_request(modbus_request *r);
int modbus_send_response(modbus_response *r);
