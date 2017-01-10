/*
 * hardware_ini.c - functions for HW initialisation
 *
 * Copyright 2014 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

/*
 * All hardware-dependent initialisation & definition should be placed here
 * and in hardware_ini.h
 *
 */

#include "main.h"
#include "hardware_ini.h"

/**
 * GPIO initialisaion: clocking + pins setup
 */
void GPIO_init(){
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    // turn on pull-up
    gpio_set(BTNS_PORT, BTN_PLUS_PIN | BTN_MINUS_PIN);
    // turn off LEDs
    gpio_set(LEDS_PORT, LED_LOW_PIN | LED_UPPER_PIN);
    // Buttons: pull-up input
    gpio_set_mode(BTNS_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
            BTN_PLUS_PIN | BTN_MINUS_PIN);
    // LEDS: opendrain output
    gpio_set_mode(LEDS_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN,
            LED_LOW_PIN | LED_UPPER_PIN);
    // Tacting output (push-pull)
    gpio_set_mode(OUTP_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
            OUTP_PIN);
/*
    // USB_DISC: push-pull
    gpio_set_mode(USB_DISC_PORT, GPIO_MODE_OUTPUT_2_MHZ,
                GPIO_CNF_OUTPUT_PUSHPULL, USB_DISC_PIN);
    // USB_POWER: open drain, externall pull down with R7 (22k)
    gpio_set_mode(USB_POWER_PORT, GPIO_MODE_INPUT,
                GPIO_CNF_INPUT_FLOAT, USB_POWER_PIN);
*/
}

/*
 * SysTick used for system timer with period of 1ms
 */
void SysTick_init(){
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8); // Systyck: 72/8=9MHz
    systick_set_reload(8999); // 9000 pulses: 1kHz
    systick_interrupt_enable();
    systick_counter_enable();
}

