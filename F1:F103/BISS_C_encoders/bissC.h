/*
 * This file is part of the encoders project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

typedef struct {
    uint32_t data;              // 26/32/36-bit data
    uint8_t error : 1;          // error flag (0 - error: bad value or high temperature)
    uint8_t warning : 1;        // warning flag (0 - warning: need to be cleaned)
    uint8_t crc_valid : 1;      // CRC validation result
    uint8_t frame_valid : 1;    // Overall frame validity
} BiSS_Frame;

BiSS_Frame parse_biss_frame(const uint8_t *bytes, uint32_t num_bytes);
