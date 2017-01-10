/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2013 Chuck McManis <cmcmanis@mcmanis.com>
 * Copyright (C) 2013 Onno Kortmann <onno@gmx.net>
 * Copyright (C) 2013 Frantisek Burian <BuFran@seznam.cz> (merge)
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

#include "stm32f0.h"

/* Called when systick fires */
void sys_tick_handler(void){
    pin_toggle(GPIOB, GPIO3);
}

/*
 * Set up timer to fire every x milliseconds
 * This is a unusual usage of systick, be very careful with the 24bit range
 * of the systick counter!  You can range from 1 to 2796ms with this.
 */
static void systick_setup(int xms){
    static int curms = 0;
    if(curms == xms) return;
    // 6MHz
    systick_set_clocksource(STK_CSR_CLKSOURCE_EXT);
    /* clear counter so it starts right away */
    STK_CVR = 0;

    systick_set_reload(6000 * xms);
    systick_counter_enable();
    systick_interrupt_enable();
    curms = xms;
}

/* set STM32 to clock by 48MHz from HSI oscillator */
static void clock_setup(void){
    rcc_clock_setup_in_hsi_out_48mhz();
    //rcc_clock_setup_in_hse_8mhz_out_48mhz();

    /* Enable clocks to the GPIO subsystems (A&B) */
    RCC_AHBENR = RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
}

static void gpio_setup(void){
    /* Set green led (PB3) as output */
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);
    /* D2 - PA12 - switch blink rate (pullup input) */
    gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO12);
}


int main(void){
    clock_setup();
    gpio_setup();

    /* 500ms ticks =>  1000ms period => 1Hz blinks */
    systick_setup(500);

    /* Do nothing in main loop */
    while (1){
        if(pin_read(GPIOA, GPIO12)) systick_setup(125); // no jumper - 4Hz
        else systick_setup(500); // 1Hz
    }
}
