/*
 * This file is part of the cordic project.
 * Copyright 2026 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

// functions
enum {
    CORDIC_CSR_FUNC_COS = 0,
    CORDIC_CSR_FUNC_SIN,
    CORDIC_CSR_FUNC_PHASE,
    CORDIC_CSR_FUNC_MOD,
    CORDIC_CSR_FUNC_ATAN,
    CORDIC_CSR_FUNC_COSH,
    CORDIC_CSR_FUNC_ATANH,
    CORDIC_CSR_FUNC_LOG,
    CORDIC_CSR_FUNC_SQRT
};

float cordic_sin(float angle);
float cordic_cos(float angle);
float cordic_atan(float val);
float cordic_sqrt(float x);
float cordic_log(float x);
