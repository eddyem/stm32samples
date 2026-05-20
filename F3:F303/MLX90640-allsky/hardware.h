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

// SPI_CS - PB9
#define SPI_CS_1()      pin_set(GPIOB, 1<<9)
#define SPI_CS_0()      pin_clear(GPIOB, 1<<9)

// interval of environment measurements, ms
#define ENV_MEAS_PERIOD     (10000)

// heater check period, ms
#define HTR_CHECK_PERIOD    (10000)

// temperature hysteresis (+-hyst from holding value)
#define HOLDT_HYSTERESIS    (5.f)
// environment for auto-heater
// maximal humidity
#define HUMIDITY_MAX        (90.f)
// delta over dew point
#define TDEW_OVER_DELTA     (7.f)
// defrosting temperature (when there's very cold)
#define TEMP_DEFROST        (5.f)
// PWM starting value (up to reaching holding T)
#define PWM_START_VAL       (100)
// middle value
#define PWM_MID_VAL         (50)
// PWM holding value (up to setTemp+Thyst)
#define PWM_HOLD_VAL        (10)

// External heater PWM: TIM3_CH1 or TIM16_CH1
// Max PWM CCR1 value (->1)
#define PWM_CCR_MAX (100)
// amount of heaters
#define HTR_AMOUNT      (2)
// PWM channels (start from 0 - CH1)
#define PWM_CH_HTR(x)   (x)
// propto external T (the higher - the brighter)
#define PWM_CH_TEXT     (2)
// propto Tsky - Text (the higher - the brighter)
#define PWM_CH_TSKY     (3)
#define PWM_CH_MAX      (3)

// amount of T channels
#define NTC_AMOUNT  (4)

typedef struct{
    float T;    // temperature, degC
    float Tdew; // dew point, degC
    float P;    // pressure, Pa
    float H;    // humidity, percents
    float Tsky; // mean Tsky, degC
    uint32_t Tmeas; // time of measurement
} bme280_t;

extern volatile uint32_t Tms;
extern uint8_t AutoHeater;

void hw_setup();
int bme_init();
void bme_process();
int get_environment(bme280_t *env);
int setPWM(uint8_t ch, uint8_t val);
