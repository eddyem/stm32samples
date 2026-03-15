/*
 * This file is part of the usbcangpio project.
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
#include <stm32f0.h>

#ifdef EBUG
#define USBIF   IGPIO
#include "strfunc.h"
#define DBG(x)  SEND(x)
#define DBGNL() NL()
#else
#define DBG(x)
#define DBGNL()
#endif


// MODER
typedef enum{
    MODE_INPUT = 0,
    MODE_OUTPUT = 1,
    MODE_AF = 2,
    MODE_ANALOG = 3
} pinmode_t;

// PUPDR
typedef enum{
    PULL_NONE = 0,
    PULL_UP = 1,
    PULL_DOWN = 2
} pinpull_t;

// OTYPER
typedef enum{
    OUTPUT_PP = 0,
    OUTPUT_OD = 1
} pinout_t;

// OSPEEDR
typedef enum{
    SPEED_LOW = 0,
    SPEED_MEDIUM = 1,
    SPEED_HIGH = 3
} pinspeed_t;

// !!! FuncNames means position of bit in funcvalues_t.flags!
typedef enum FuncNames{ // shift 1 by this to get "canUSART" etc; not more than 7!
    FUNC_AIN = 0,
    FUNC_USART = 1,
    FUNC_SPI = 2,
    FUNC_I2C = 3,
    FUNC_PWM = 4,
    FUNC_AMOUNT // just for arrays' sizes
} funcnames_t;
/*
typedef union{
    struct{
        uint8_t canADC : 1;
        uint8_t canUSART : 1;
        uint8_t canSPI : 1;
    };
    uint8_t flags;
} funcvalues_t;
*/
typedef struct{
    uint8_t enable : 1;  // [immutable!] pin config avialable (==1 for PA0-PA3, PA5-PA7, PA9, PA10, PB0-PB7, PB10, PB11, ==0 for rest)
    pinmode_t mode : 2;
    pinpull_t pull : 2;
    pinout_t otype : 1;
    pinspeed_t speed : 2;
    uint8_t afno : 3;    // alternate function number (only if mode == MODE_AF)
    funcnames_t af : 3;  // alternate function name (`FuncNames`)
    uint8_t monitor : 1; // monitor changes
    uint16_t threshold;  // threshold for ADC measurement
} pinconfig_t;

/*
typedef struct{
    uint32_t speed;
    uint8_t cpol : 1;
    uint8_t cpha : 1;
    uint8_t lsbfirst : 1;
    uint8_t enabled : 1;
} spiconfig_t;
*/

int is_disabled(uint8_t port, uint8_t pin);
int chkpinconf();

int set_pinfunc(uint8_t port, uint8_t pin, pinconfig_t *pcfg);
int get_curpinconf(uint8_t port, uint8_t pin, pinconfig_t *c);

int gpio_reinit();

int pin_out(uint8_t port, uint8_t pin, uint8_t newval);
int16_t pin_in(uint8_t port, uint8_t pin);
uint16_t gpio_alert(uint8_t port);
