/*
 * timer.c
 *
 * Copyright 2016 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include "timer.h"
#include "user_proto.h" // for print_int

// current speed
int32_t current_RPM = 0;
void get_RPM();
uint16_t get_ARR(int32_t RPM);

// pulses: 16 1/0, 4 1/1, 16 1/0, 4 0/0,
const uint8_t pulses[] = {
    1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,
    1,1,1,1,1,1,1,1,
    1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,
    0,0,0,0,0,0,0,0};
const int pulsesLen = sizeof(pulses) - 1;

void tim2_init(){
    // init TIM2
    rcc_periph_clock_enable(RCC_TIM2);
    timer_reset(TIM2);
    // timer have frequency of 1MHz
    timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    // 72MHz div 36 = 2MHz
    TIM2_PSC = 35;  // we need double freq to serve both halfs of signal
    TIM2_ARR = get_ARR(MIN_RPM);
    TIM2_DIER = TIM_DIER_UDE | TIM_DIER_UIE;
    nvic_enable_irq(NVIC_TIM2_IRQ);
    TIM2_CR1 |= TIM_CR1_CEN;
    get_RPM();
}

void tim2_isr(){
    static uint16_t ctr = 0;
    if(TIM2_SR & TIM_SR_UIF){ // update interrupt
        if(ctr > pulsesLen) ctr = 0;
        if(pulses[++ctr])
            GPIO_BSRR(OUTP_PORT) = OUTP_PIN;
        else
            GPIO_BSRR(OUTP_PORT) = OUTP_PIN << 16;
        TIM2_SR = 0;
    }
}

/**
 * Calculate motor speed in RPM
 * RPM = 1/tim2_arr / 40 * 60
 */
void get_RPM(){
    current_RPM = 3000000 / (int32_t)TIM2_ARR;
    current_RPM /= 2;
}

// calculate TIM2_ARR by RPM
uint16_t get_ARR(int32_t RPM){
    int32_t R = 3000000 / RPM;
    R /= 2;
    return (uint16_t)R;
}

/**
 * Change "rotation speed" by 100rpm
 */
void increase_speed(){
    if(current_RPM == MAX_RPM) return;
    current_RPM += 100;
    if(current_RPM > MAX_RPM){ // set LED "MAX"
        current_RPM = MAX_RPM;
        TIM2_ARR = get_ARR(current_RPM);
        gpio_clear(LEDS_PORT, LED_UPPER_PIN);
    }else{
        TIM2_ARR = get_ARR(current_RPM);
        get_RPM(); // recalculate speed
        gpio_set(LEDS_PORT, LED_UPPER_PIN);
    }
    gpio_set(LEDS_PORT, LED_LOW_PIN);
    print_int(current_RPM);
}

void decrease_speed(){
    if(current_RPM == MIN_RPM) return;
    current_RPM -= 100;
    if(current_RPM < MIN_RPM){ // set LED "MIN"
        current_RPM = MIN_RPM;
        TIM2_ARR = get_ARR(current_RPM);
        gpio_clear(LEDS_PORT, LED_LOW_PIN);
    }else{
        TIM2_ARR = get_ARR(current_RPM);
        get_RPM();
        gpio_set(LEDS_PORT, LED_LOW_PIN);
    }
    gpio_set(LEDS_PORT, LED_UPPER_PIN);
    print_int(current_RPM);
}
