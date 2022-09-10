/*
 * This file is part of the canonmanage project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

// all data sent in big-endian format

// max length of commands
#define MAXCMDLEN   (32)

// answer for cmd "POLL"
#define CANON_POLLANS   (0xaa)

typedef enum{
    CANON_LONGID    = 0x00, // long ID ???
    CANON_ID        = 0x01, // lens ID and other info ???
    CANON_REPDIA    = 0x02, // repeat last diaphragm change
    CANON_FSTOP     = 0x04, // stop focus changing
    CANON_FMAX      = 0x05, // set Foc to max
    CANON_FMIN      = 0x06, // =//= min
    CANON_POWERON   = 0x07, // turn on motors' power
    CANON_POWEROFF  = 0x08, // turn off power
    CANON_POLL      = 0x0a, // bysu poll (ans 0xaa when ready or last command code)
    CANON_DIAPHRAGM = 0x13, // open/close diaphragm by given (int8_t) value of steps
    CANON_FOCMOVE   = 0x44, // move focus dial by given amount of steps (int16)
    CANON_FOCBYHANDS= 0x5e, // turn on focus move by hands (to turn off send 4,5 or 6)
    CANON_GETINFO   = 0x80, // get information
    CANON_GETREG    = 0x90, // get regulators' state
    CANON_GETMODEL  = 0xa0, // get lens (e.g. 200 == LX200)
    CANON_GETDIAL   = 0xc0, // get focus dial position in steps
    CANON_GETFOCM   = 0xc2, // get focus position in meters (not for all lenses)
} canon_commands;

void canon_init();
void canon_proc();
int canon_diaphragm(char command);
int canon_focus(int16_t val);
int canon_sendcmd(uint8_t cmd);
int canon_asku16(uint8_t cmd);
void canon_setlastcmd(uint8_t x);
int canon_getinfo();
