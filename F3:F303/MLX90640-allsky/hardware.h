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

// External heater PWM: TIM3_CH1 or TIM16_CH1
// Max PWM CCR1 value (->1)
#define PWM_CCR_MAX (100)
// PWM channels (start from 0 - CH1)
// external heater
#define PWM_CH_HEATER   (0)
// propto humidity (the higher - the brighter)
#define PWM_CH_HUMIDITY (1)
// propto external T (the higher - the brighter)
#define PWM_CH_TEXT     (2)
// propto Tsky - Text (the higher - the brighter)
#define PWM_CH_TSKY     (3)

typedef struct{
    float T;    // temperature, degC
    float Tdew; // dew point, degC
    float P;    // pressure, Pa
    float H;    // humidity, percents
    float Tsky; // mean Tsky, degC
    // TODO: add here values of NTC on ADC channels 1/2
    uint32_t Tmeas; // time of measurement
} bme280_t;

extern volatile uint32_t Tms;

void hw_setup();
int bme_init();
void bme_process();
int get_environment(bme280_t *env);
int setPWM(uint8_t ch, uint8_t val);
