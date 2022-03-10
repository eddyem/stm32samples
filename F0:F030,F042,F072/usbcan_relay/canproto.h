/*
 * This file is part of the canrelay project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#ifndef CANPROTO_H__
#define CANPROTO_H__

#include "stm32f0.h"
#include "can.h"

// command without setter flag
#define CANCMDMASK  (0x7f)

typedef enum{
    CAN_CMD_PING,   // ping (without setter)
    CAN_CMD_RELAY,  // relay get/set
    CAN_CMD_PWM,    // PWM get/set
    CAN_CMD_ADC,    // get ADC (without setter)
    CAN_CMD_MCU,    // MCU T and Vdd
    CAN_CMD_LED,    // LEDs get/set
    CAN_CMD_BTNS,   // get Buttons state (without setter)
    CAN_CMD_TMS,    // get time from start (in ms)
    CAN_CMD_ERRCMD, // wrong command
    CAN_CMD_SETFLAG = 0x80 // command is setter
} CAN_commands;

// output messages identifier
#define OUTPID  (CANID)

void parseCANcommand(CAN_message *msg);

#endif // CANPROTO_H__
