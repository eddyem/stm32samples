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

#include "flash.h"
#include "gpio.h"

// TODO: remove AFmask, make function to get right AF number by pin's FuncValues
typedef struct{
    funcvalues_t vals;
    uint8_t AFmask;
} pinprops_t;

static const pinprops_t pin_props[2][16] = {
    [0] = { // PORT A
        [0]  = { .vals.flags = 0b00000001, .AFmask = (1<<2) }, // PA0: ADC0, AF2 (TIM2_CH1)
        [1]  = { .vals.flags = 0b00000001, .AFmask = (1<<2) }, // PA1: ADC1, AF2 (TIM2_CH2)
        [2]  = { .vals.flags = 0b00000011, .AFmask = (1<<2) | (1<<1) }, // PA2: ADC2, AF2 (TIM2_CH3), AF1 (USART2_TX)
        [3]  = { .vals.flags = 0b00000011, .AFmask = (1<<2) | (1<<1) }, // PA3: ADC3, AF2 (TIM2_CH4), AF1 (USART2_RX)
        [5]  = { .vals.flags = 0b00000101, .AFmask = (1<<0) }, // PA5: ADC5, SPI1_SCK (AF0)
        [6]  = { .vals.flags = 0b00000101, .AFmask = (1<<0) }, // PA6: ADC6, SPI1_MISO (AF0)
        [7]  = { .vals.flags = 0b00000101, .AFmask = (1<<0) }, // PA7: ADC7, SPI1_MOSI (AF0)
        [9]  = { .vals.flags = 0b00000010, .AFmask = (1<<1) }, // PA9: USART1_TX (AF1)
        [10] = { .vals.flags = 0b00000010, .AFmask = (1<<1) }, // PA10: USART1_RX (AF1)
    },
    [1] = { // PORT B
        [0]  = { .vals.flags = 0b00000001, .AFmask = (1<<2) | (1<<3) }, // PB0: ADC8, TIM3_CH3 (AF1), TIM1_CH2N (AF2)
        [1]  = { .vals.flags = 0b00000001, .AFmask = (1<<0) | (1<<1) | (1<<2) }, // PB1: ADC9, TIM14_CH1 (AF0), TIM3_CH4 (AF1), TIM1_CH3N (AF2)
        [2]  = { .vals.flags = 0b00000000, .AFmask = 0 }, // PB2: nothing except GPIO
        [3]  = { .vals.flags = 0b00000100, .AFmask = (1<<0) | (1<<2) }, // PB3: SPI1_SCK (AF0), TIM2_CH2 (AF2)
        [4]  = { .vals.flags = 0b00000100, .AFmask = (1<<0) | (1<<1) }, // PB4: SPI1_MISO (AF0), TIM3_CH1 (AF1)
        [5]  = { .vals.flags = 0b00000100, .AFmask = (1<<0) | (1<<1) }, // PB5: SPI1_MOSI (AF0), TIM3_CH2 (AF1)
        [6]  = { .vals.flags = 0b00000010, .AFmask = (1<<0) | (1<<1) | (1<<2) }, // PB6: USART1_TX (AF0), I2C1_SCL (AF1), TIM16_CH1N (AF2)
        [7]  = { .vals.flags = 0b00000010, .AFmask = (1<<0) | (1<<1) | (1<<2) }, // PB7: USART1_RX (AF0), I2C1_SDA (AF1), TIM17_CH1N (AF2)
        [10] = { .vals.flags = 0b00000000, .AFmask = (1<<1) | (1<<2) }, // PB10: I2C1_SCL (AF1), TIM2_CH3 (AF2)
        [11] = { .vals.flags = 0b00000000, .AFmask = (1<<1) | (1<<2) }, // PB11: I2C1_SDA (AF1), TIM2_CH4 (AF2)
    }
};

static int is_disabled(uint8_t port, uint8_t pin){
    if(port > 1 || pin > 15) return FALSE;
    if(!the_conf.pinconfig[port][pin].enable) return FALSE;
    return TRUE;
}

/**
 * @brief is_func_allowed - check if alternate function `afno` allowed on given pin
 * @param port - 0 for GPIOA and 1 for GPIOB
 * @param pin - 0..15
 * @param afno - number of alternate function
 * @return TRUE if all OK
 */
int is_func_allowed(uint8_t port, uint8_t pin, pinconfig_t *pcfg){
    if(is_disabled(port, pin)) return FALSE;
    const pinprops_t *props = &pin_props[port][pin];
    switch(pcfg->mode){
    case MODE_ANALOG:
        if(!props->vals.canADC) return FALSE;
        pcfg->pull = PULL_NONE; // no PullUp for analog mode
        break;
    case MODE_AF:
        // here af is one of enum FuncValues !!! we should change `af` later
        if(!((1<<pcfg->af) & props->vals.flags)) return FALSE;
        // TODO: set right AF number here !!!
        //if(!(props->AFmask & (1 << pcfg->af))) return FALSE; // no such AF or not supported
        pcfg->speed = SPEED_HIGH; // many AF needs high speed
        pcfg->otype = OUTPUT_PP; // no OD for AF
        break;
    case MODE_INPUT: // no limits
        break;
    case MODE_OUTPUT: // no limits
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

int gpio_reinit(){
    for(int port = 0; port < 2; port++){
        GPIO_TypeDef *gpio = (port == 0) ? GPIOA : GPIOB;
        for(int pin = 0; pin < 16; pin++){
            pinconfig_t *cfg = &the_conf.pinconfig[port][pin];
            int shift2 = pin << 1;
            if(!cfg->enable) continue;
            gpio->MODER = (gpio->MODER & ~(3 << shift2))| (cfg->mode << shift2);
            gpio->OTYPER = (gpio->OTYPER & ~(1 << pin)) | (cfg->otype << pin);
            gpio->OSPEEDR = (gpio->OSPEEDR & ~(3 << shift2)) | (cfg->speed << shift2);
            gpio->PUPDR = (gpio->PUPDR & ~(3 << shift2)) | (cfg->pull << shift2);
            if(pin < 8){
                int shift4 = pin << 4;
                gpio->AFR[0] = (gpio->AFR[0] & ~(0xf << shift4)) | (cfg->af << shift4);
            }else{
                int shift4 = (pin - 8) << 4;
                gpio->AFR[1] = (gpio->AFR[1] & ~(0xf << shift4)) | (cfg->af << shift4);
            }
        }
    }
    // TODO: configure USART, SPI etc
    return TRUE;
}
