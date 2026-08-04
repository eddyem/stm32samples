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

// libmath tests
uint32_t test_math_sin();
uint32_t test_math_cos();
uint32_t test_math_atan();
uint32_t test_math_sqrt();
uint32_t test_math_log();

// CORDIC tests
uint32_t test_cordic_sin();
uint32_t test_cordic_cos();
uint32_t test_cordic_atan();
uint32_t test_cordic_sqrt();
uint32_t test_cordic_log();
