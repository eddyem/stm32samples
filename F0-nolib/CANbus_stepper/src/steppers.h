/*
 * This file is part of the Stepper project.
 * Copyright 2020 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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
#ifndef STEPPERS_H__
#define STEPPERS_H__

#include <stm32f0.h>

typedef enum{
    DRV_NONE,   // driver is absent
    DRV_NOTINIT,// not initialized
    DRV_MAILF,  // mailfunction - no Vdd when Vio_ON activated
    DRV_8825,   // DRV8825 connected
    DRV_4988,   // A4988 connected
    DRV_2130    // TMC2130 connected
} drv_type;

drv_type initDriver();
drv_type getDrvType();

#endif // STEPPERS_H__
