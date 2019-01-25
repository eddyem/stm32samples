/*
 * This file is part of the Chiller project.
 * Copyright 2019 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#include "stm32f0.h"

// temperature setpoint
extern int16_t Tset;

// temperatures of NTC
extern int16_t NTCval[4];
// meaning of each array member: in/out, heater and air
#define TI_IDX              (0)
#define TO_IDX              (1)
#define TH_IDX              (2)
#define TA_IDX              (3)
#define INPUT_TEMPERATURE   NTCval[TI_IDX]
#define OUTPUT_TEMPERATURE  NTCval[TO_IDX]
#define HEATER_TEMPERATURE  NTCval[TH_IDX]
#define AIR_TEMPERATURE     NTCval[TA_IDX]


/* status bits */
// ==1 if no changes
#define ST_OK               (1<<0)
// (ST_OK=0) == 1 if moving faster (or hotter), 0 if slower (or cooler)
#define ST_FASTER           (1<<1)
// turn OFF
#define ST_OFF              (1<<2)
// critical error
#define ST_CRITICAL         (1<<7)

/* chiller status codes */
typedef struct{
    uint8_t common_state; // common state != ST_OK if some other states changed
    uint8_t heater_state;
    uint8_t cooler_state;
    uint8_t pump_state;
} chiller_state;

chiller_state *mainloop();

