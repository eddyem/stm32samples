/*
 * This file is part of the nitrogen project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

// temporary defines - should be stored in settings
// temperature (degC) limits
#define T_MIN       (-20.f)
#define T_MAX       (40.f)
// pressure (Pa) limit
#define P_MAX       (120e3f)
// humidity limit
#define H_MAX       (90.f)
// minimal difference above dew point
#define DEW_MIN     (3.f)

void indication_process();
