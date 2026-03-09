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

#include <stm32f0.h>
#include <string.h>

#include "flash.h"
#include "gpio.h"

static uint16_t monitor_mask[2] = {0}; // pins to monitor == 1 (ONLY GPIO!!!)
static uint16_t oldstates[2][16] = {0}; // previous state (16 bits - as some pins could be analog)

// strings for keywords
const char *str_keywords[] = {
#define KW(x)    [STR_ ## x] = #x,
    KEYWORDS
#undef KW
};

// TODO: remove AFmask, make function to get right AF number by pin's FuncValues
typedef struct{
    uint8_t funcs;  // bitmask according to enum FuncNames
    uint8_t AF[FUNC_AMOUNT]; // alternate function number for corresponding `FuncNames` number
} pinprops_t;

#define CANADC(x)   ((x) & (1<<FUNC_AIN))
#define CANUSART(x) ((x) & (1<<FUNC_USART))
#define CANSPI(x)   ((x) & (1<<FUNC_SPI))
#define CANI2C(x)   ((x) & (1<<FUNC_I2C))

// AF for USART, SPI, I2C:
#define _U(x)  [FUNC_USART] = x
// _S(0) or _U(0) have no sence, but lets understand that this pin have SPI or USART
#define _S(x)  [FUNC_SPI] = x
#define _I(x)  [FUNC_I2C] = x
static const pinprops_t pin_props[2][16] = {
    [0] = { // PORT A
        [0]  = { .funcs = 0b00000001, .AF = {0}}, // PA0: ADC0, AF2 (TIM2_CH1)
        [1]  = { .funcs = 0b00000001, .AF = {0}}, // PA1: ADC1, AF2 (TIM2_CH2)
        [2]  = { .funcs = 0b00000011, .AF = {_U(1)}}, // PA2: ADC2, AF2 (TIM2_CH3), AF1 (USART2_TX)
        [3]  = { .funcs = 0b00000011, .AF = {_U(1)}}, // PA3: ADC3, AF2 (TIM2_CH4), AF1 (USART2_RX)
        [5]  = { .funcs = 0b00000101, .AF = {_S(0)}}, // PA5: ADC5, SPI1_SCK (AF0)
        [6]  = { .funcs = 0b00000101, .AF = {_S(0)}}, // PA6: ADC6, SPI1_MISO (AF0)
        [7]  = { .funcs = 0b00000101, .AF = {_S(0)}}, // PA7: ADC7, SPI1_MOSI (AF0)
        [9]  = { .funcs = 0b00000010, .AF = {_U(1)}}, // PA9: USART1_TX (AF1)
        [10] = { .funcs = 0b00000010, .AF = {_U(1)}}, // PA10: USART1_RX (AF1)
    },
    [1] = { // PORT B
        [0]  = { .funcs = 0b00000001, .AF = {0}}, // PB0: ADC8, TIM3_CH3 (AF1), TIM1_CH2N (AF2)
        [1]  = { .funcs = 0b00000001, .AF = {0}}, // PB1: ADC9, TIM14_CH1 (AF0), TIM3_CH4 (AF1), TIM1_CH3N (AF2)
        [2]  = { .funcs = 0b00000000, .AF = {0}}, // PB2: nothing except GPIO
        [3]  = { .funcs = 0b00000100, .AF = {_S(0)}}, // PB3: SPI1_SCK (AF0), TIM2_CH2 (AF2)
        [4]  = { .funcs = 0b00000100, .AF = {_S(0)}}, // PB4: SPI1_MISO (AF0), TIM3_CH1 (AF1)
        [5]  = { .funcs = 0b00000100, .AF = {_S(0)}}, // PB5: SPI1_MOSI (AF0), TIM3_CH2 (AF1)
        [6]  = { .funcs = 0b00001010, .AF = {_U(0), _I(1)}}, // PB6: USART1_TX (AF0), I2C1_SCL (AF1), TIM16_CH1N (AF2)
        [7]  = { .funcs = 0b00001010, .AF = {_U(0), _I(1)}}, // PB7: USART1_RX (AF0), I2C1_SDA (AF1), TIM17_CH1N (AF2)
        [10] = { .funcs = 0b00001000, .AF = {_I(1)}}, // PB10: I2C1_SCL (AF1), TIM2_CH3 (AF2)
        [11] = { .funcs = 0b00001000, .AF = {_I(1)}}, // PB11: I2C1_SDA (AF1), TIM2_CH4 (AF2)
    }
};
#undef _U
#undef _S
#undef _I

static int is_disabled(uint8_t port, uint8_t pin){
    if(port > 1 || pin > 15) return FALSE;
    if(the_conf.pinconfig[port][pin].enable) return FALSE;
    return TRUE;
}

/**
 * @brief set_pinfunc - check if alternate function `afno` allowed on given pin
 * @param port - 0 for GPIOA and 1 for GPIOB
 * @param pin - 0..15
 * @param pcfg (io) - pin configuration
 * @return TRUE if all OK
 */
int set_pinfunc(uint8_t port, uint8_t pin, pinconfig_t *pcfg){
    DBG("set_pinfunc()\n");
    if(is_disabled(port, pin) || !pcfg){
        DBG("Disabled?\n");
        return FALSE;
    }
    const pinprops_t *props = &pin_props[port][pin];
    switch(pcfg->mode){
    case MODE_ANALOG:
        DBG("Analog\n");
        if(!CANADC(props->funcs)){
            DBG("Can't ADC\n");
            return FALSE;
        }
        pcfg->pull = PULL_NONE; // no PullUp for analog mode
        break;
    case MODE_AF:
        DBG("Altfun\n");
        // here af is one of enum FuncValues !!! we should change `af` later
        if(pcfg->af >= FUNC_AMOUNT || !((1<<pcfg->af) & props->funcs)){
            DBG("Wrong AF\n");
            return FALSE;
        }
        pcfg->afno = props->AF[pcfg->af];
        pcfg->speed = SPEED_HIGH; // many AF needs high speed
        pcfg->otype = OUTPUT_PP; // no OD for AF
        break;
    case MODE_INPUT: // no limits
        DBG("Input\n");
        break;
    case MODE_OUTPUT: // remove pullup/pulldown for PP
        DBG("Output\n");
        if(pcfg->otype == OUTPUT_PP) pcfg->pull = PULL_NONE;
        break;
    default:
        DBG("Wrong\n");
        return FALSE;
    }
    pcfg->enable = 1; // don't forget to set enable flag!
    the_conf.pinconfig[port][pin] = *pcfg;
    DBG("All OK\n");
    return TRUE;
}

// reinit all GPIO registers due to config; also configure (if need) USART1/2, SPI1 and I2C1
int gpio_reinit(){
    bzero(monitor_mask, sizeof(monitor_mask));
    bzero(oldstates, sizeof(oldstates));
    for(int port = 0; port < 2; port++){
        GPIO_TypeDef *gpio = (port == 0) ? GPIOA : GPIOB;
        for(int pin = 0; pin < 16; pin++){
            pinconfig_t *cfg = &the_conf.pinconfig[port][pin];
            if(!cfg->enable) continue;
            const pinprops_t *props = &pin_props[port][pin];
            if(cfg->mode == MODE_AF && (cfg->af >= FUNC_AMOUNT ||
                !((1<<cfg->af) & props->funcs) ||
                (cfg->afno != props->AF[cfg->af]))){ // wrong configuration -> don't mind AF, make FLIN
                DBG("Wrong AF config -> FL IN\n");
                cfg->af = FUNC_AIN;
                cfg->afno = 0;
                cfg->mode = MODE_INPUT;
                cfg->monitor = 0;
                cfg->speed = SPEED_LOW;
                cfg->pull = PULL_NONE;
            }
            int shift2 = pin << 1;
            gpio->MODER = (gpio->MODER & ~(3 << shift2))| (cfg->mode << shift2);
            gpio->OTYPER = (gpio->OTYPER & ~(1 << pin)) | (cfg->otype << pin);
            gpio->OSPEEDR = (gpio->OSPEEDR & ~(3 << shift2)) | (cfg->speed << shift2);
            gpio->PUPDR = (gpio->PUPDR & ~(3 << shift2)) | (cfg->pull << shift2);
            if(pin < 8){
                int shift4 = pin << 4;
                gpio->AFR[0] = (gpio->AFR[0] & ~(0xf << shift4)) | (cfg->afno << shift4);
            }else{
                int shift4 = (pin - 8) << 4;
                gpio->AFR[1] = (gpio->AFR[1] & ~(0xf << shift4)) | (cfg->afno << shift4);
            }
            if(cfg->monitor && cfg->mode != MODE_AF) monitor_mask[port] |= (1 << pin);
        }
    }
    // TODO: configure USART, SPI etc
    // also chech cfg->monitor!
    return TRUE;
}

// get MODER for current pin
TRUE_INLINE uint32_t get_moder(volatile GPIO_TypeDef * GPIOx, uint8_t pin){
    return (GPIOx->MODER >> (pin << 1)) & 3;
}

/**
 * @brief pin_out - change pin  value
 * @param port - 0 for GPIOA, 1 for GPIOB
 * @param pin - 0..15
 * @param newval - 0 or 1 (reset/set)
 * @return FALSE if pin isn't OUT or other err
 * here I check real current settings by GPIOx->MODER
 */
int pin_out(uint8_t port, uint8_t pin, uint8_t newval){
    if(port > 1 || pin > 15) return FALSE;
    volatile GPIO_TypeDef * GPIOx = (port == 0) ? GPIOA : GPIOB;
    uint16_t mask = 1 << pin;
    uint32_t moder = get_moder(GPIOx, pin);
    if(moder != MODE_OUTPUT) return FALSE;
    if(newval) GPIOx->BSRR = mask;
    else GPIOx->BRR = mask;
    return TRUE;
}

/**
 * @brief pin_in - get current pin's value (0/1 for regular GPIO, 0..4095 for ADC)
 * @param port - 0..1
 * @param pin - 0..15
 * @return value or -1 if pin have AF or don't used
 */
int16_t pin_in(uint8_t port, uint8_t pin){
    if(port > 1 || pin > 15) return -1;
    volatile GPIO_TypeDef * GPIOx = (port == 0) ? GPIOA : GPIOB;
    uint32_t moder = get_moder(GPIOx, pin);
    int16_t val = -1;
    switch(moder){ // check REAL pin config
    case MODE_INPUT:
    case MODE_OUTPUT:
        if(GPIOx->IDR & (1<<pin)) val = 1;
        else val = 0;
    break;
        // case MODE_ANALOG:
    default: // TODO: add ADC!
        break;
    }
    return val;
}

/**
 * @brief gpio_alert - return bitmask for alerted pins (whos state changed over last check)
 *      AF don't checked! Use appropriate function for them
 * @param port - 0 for GPIOA, 1 for GPIOB
 * @return pin mask where 1 is for changed state
 */
uint16_t gpio_alert(uint8_t port){
    if(port > 1) return 0;
    if(0 == monitor_mask[port]) return 0; // nothing to monitor
    volatile GPIO_TypeDef * GPIOx = (port == 0) ? GPIOA : GPIOB;
    uint32_t moder = GPIOx->MODER;
    uint16_t curpinbit = 1; // shift each iteration
    uint16_t *oldstate = oldstates[port];
    uint16_t alert = 0;
    for(int pin = 0; pin < 16; ++pin, curpinbit <<= 1, moder >>= 2){
        uint8_t curm = moder & 3;
        if((curm == MODE_AF) || 0 == (monitor_mask[port] & curpinbit)) continue; // monitor also OUT (if OD)
        // TODO: add AIN
        if(curm == MODE_ANALOG) continue;
        uint16_t curval = (GPIOx->IDR & curpinbit) ? 1 : 0;
        if(oldstate[pin] != curval){
            oldstate[pin] = curval;
            alert |= curpinbit;
        }
    }
    return alert;
}
