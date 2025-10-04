/*
 * This file is part of the ir-allsky project.
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

#include <stm32f3.h>

#define USBPU_port  GPIOA
#define USBPU_pin   (1<<15)
#define USBPU_ON()  pin_clear(USBPU_port, USBPU_pin)
#define USBPU_OFF() pin_set(USBPU_port, USBPU_pin)

// SPI_CS - PB2
#define SPI_CS_1()      pin_set(GPIOB, 1<<2)
#define SPI_CS_0()      pin_clear(GPIOB, 1<<2)

// interval of environment measurements, ms
#define ENV_MEAS_PERIOD (10000)

typedef struct{
    float T;    // temperature, degC
    float Tdew; // dew point, degC
    float P;    // pressure, Pa
    float H;    // humidity, percents
    uint32_t Tmeas; // time of measurement
} bme280_t;

extern volatile uint32_t Tms;

void hw_setup();
int bme_init();
void bme_process();
int get_environment(bme280_t *env);
