/*
 * This file is part of the servo project.
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

// Default starting values
// minimal and maximal pulse length for SG90
#define SG90_MINPULSE   400
#define SG90_MAXPULSE   2600
#define SG90_MIDPULSE   ((SG90_MINPULSE+SG90_MAXPULSE)/2)
#define SG90_AMPL       (SG90_MAXPULSE-SG90_MINPULSE)
// maximal speed: 0.1s (5 ticks) per 60degr (1/3 of range):  (SG90_AMPL/15)
#define SG90_MAXSPEED   93

// Limiting values
#define SERVO_MINPULSE  100
#define SERVO_MAXPULSE  5000
#define SERVO_MAXSPEED  300

void process_servo();
bool servo_set_speed(uint8_t N, uint16_t s);
bool servo_get_speed(uint8_t N, uint16_t *s);
bool servo_set_tagpos(uint8_t N, uint16_t pos);
bool servo_get_tagpos(uint8_t N, uint16_t *pos);
