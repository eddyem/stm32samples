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

void set_sincos(int iscordic);
int get_sincos();
float MJD_from_unix(uint32_t t);
//float LST_from_mjd(float mjd);
float LST_from_unix(uint32_t t);
float ha_to_ra(float ha, float lst);
float ra_to_ha(float ra, float lst);
void altaz_to_hadec(float alt_deg, float az_deg, float *ha_deg, float *dec_deg);
void hadec_to_altaz(float ha_deg, float dec_deg, float *alt_deg, float *az_deg);
float refraction(float phpa, float tc, float rh, float Z_rad);
