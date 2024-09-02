/*
 * This file is part of the chronometer project.
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
#pragma once
#ifndef LIDAR_H__
#define LIDAR_H__
#include <stm32f1.h>

#define LIDAR_FRAME_LEN     (9)
// frame header
#define LIDAR_FRAME_HEADER  (0x59)
// lower strength limit
#define LIDAR_LOWER_STREN   (10)
// triggered distance threshold - 1 meter
#define LIDAR_DIST_THRES    (100)
#define LIDAR_MIN_DIST      (50)
#define LIDAR_MAX_DIST      (1000)

extern uint16_t last_lidar_dist;
extern uint16_t lidar_triggered_dist;
extern uint16_t last_lidar_stren;

uint8_t parse_lidar_data(char *txt);

#endif // LIDAR_H__
