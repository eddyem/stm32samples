/*
 * This file is part of the cordic project.
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

#include <math.h>
#include <stdint.h>
#include <stm32g4.h>

#include "astro.h"
#include "test.h"
#include "hardware.h"
#include "cordic.h"

// amount of iterations over test
#define N_TESTS     1000

static float arr[N_TESTS];


// RNG
static uint32_t rand_state = 123456789;
static uint32_t next_rand(){
    rand_state = rand_state * 1664525 + 1013904223;
    return rand_state;
}

// fill array with random angles
static void fill_random_sin_cos(){
    for(int i = 0; i < N_TESTS; ++i){
        // angle from -pi to +pi
        arr[i] = (float)(next_rand() % 62831853) / 10000000.0f - 3.14159265f;
    }
}

static void fill_random_atan(){
    for(int i = 0; i < N_TESTS; ++i){
        arr[i] = (float)(next_rand() % 2000000) / 1e6f - 1e6f; // [-1,1]
    }
}

static void fill_random_sqrt(){
    for(int i = 0; i < N_TESTS; ++i){
        arr[i] = (float)(next_rand() % 10000) / 100.0f; // [0,100]
    }
}

static void fill_random_log(){
    for(int i = 0; i < N_TESTS; ++i){
        arr[i] = (float)(next_rand() % 10000 + 1) / 100.0f; // [0.01, 100]
    }
}

// main test template
static uint32_t run_test(void (*gen)(), float (*func)(float)){
    gen();
    volatile float result = 0.0f; // don't let gcc to optimize this cycle
    timer_start();
    for(int i = 0; i < N_TESTS; ++i){
        result = func(arr[i]);
        (void) result;
    }
    timer_stop();
    return timer_read();
}

static uint32_t run_test2(void (*gen)(), void (*func)(float, float*, float*)){
    gen();
    volatile float result1 = 0.f, result2 = 0.f; // don't let gcc to optimize this cycle
    timer_start();
    for(int i = 0; i < N_TESTS; ++i){
        func(arr[i], (float*)&result1, (float*)&result2);
        (void) result1;
        (void) result2;
    }
    timer_stop();
    return timer_read();
}

// ------------- math.h tests -------------
uint32_t test_math_sin(){ // 0.9us per cycle
    return run_test(fill_random_sin_cos, sinf);
}
uint32_t test_math_cos(){ // 0.9us per cycle
    return run_test(fill_random_sin_cos, cosf);
}
uint32_t test_math_atan(){ // 0.9us per cycle
    return run_test(fill_random_atan, atanf);
}
uint32_t test_math_sqrt(){ // 0.16us per cycle
    return run_test(fill_random_sqrt, sqrtf);
}
uint32_t test_math_log(){ // 0.9us per cycle
    return run_test(fill_random_log, logf);
}

// ------------- CORDIC tests -------------
uint32_t test_cordic_sincos(){ // 0.4us per cycle
    return run_test2(fill_random_sin_cos, cordic_sincos);
}
uint32_t test_cordic_sin(){ // 0.4us per cycle
    return run_test(fill_random_sin_cos, cordic_sin);
}
uint32_t test_cordic_cos(){ // 0.4us per cycle
    return run_test(fill_random_sin_cos, cordic_cos);
}
uint32_t test_cordic_atan(){ // 0.12us per cycle
    return run_test(fill_random_atan, cordic_atan);
}
uint32_t test_cordic_sqrt(){ // 0.4us per cycle
    return run_test(fill_random_sqrt, cordic_sqrt);
}
uint32_t test_cordic_log(){ // 0.4us per cycle
    return run_test(fill_random_log, cordic_log);
}

// ------------- Astronomy tests -------------
// test coordinates transformation: hor2eq and eq2hor
uint32_t test_astro_coordsTransform(){
    volatile float az = 11.3f, alt = 89.3f, ha, dec;
    timer_start();
    for(int i = 0; i < N_TESTS; ++i){
        // 160us per cycle for math.h sin/cos
        // 15us per cycle for CORDIC sin/cos
        altaz_to_hadec(alt, az, (float*)&ha, (float*)&dec);
        hadec_to_altaz(ha, dec, (float*)&alt, (float*)&az);
        if((az += 9.51f) > 359.99f) az -= 359.9f;
        if((alt -= 1.76f) < 9.9f) alt += 79.f;
        (void) ha;
        (void) dec;
    }
    timer_stop();
    return timer_read();
}

// test refraction correction for HA-DEC (eq2hor->refr->hor2eq)
uint32_t test_astro_refraction(){
    float az = 11.3f, alt = 89.3f, ha, dec;
    float phpa = 800.f, tc = 10.f, rh = 0.7f;
    volatile float newha, newdec, newalt;
    timer_start();
    for(int i = 0; i < N_TESTS; ++i){
        // 260us per cycle for math.h sin/cos
        // 34us per cycle for CORDIC sin/cos
        altaz_to_hadec(alt, az, &ha, &dec);
        hadec_to_altaz(ha, dec, &alt, &az);
        newalt = alt + refraction(phpa, tc, rh, 90.f-alt);
        altaz_to_hadec(newalt, az, (float*)&newha, (float*)&newdec);
        (void) newha;
        (void) newdec;
        if((az += 9.51f) > 359.99f) az -= 359.9f;
        if((alt -= 1.76f) < 9.9f) alt += 79.f;
    }
    timer_stop();
    return timer_read();
}

// test LST calculation
uint32_t test_astro_LST(){
    uint32_t *uarr = (uint32_t*) arr;
    volatile float result;
    for(int i = 0; i < N_TESTS; ++i){ // fill random data in 21th century
        uarr[i] = next_rand() % 3155673599 + 978307200;
    }
    timer_start();
    for(int i = 0; i < N_TESTS; ++i){ // 1.5us per cycle
        result = LST_from_unix(uarr[i]);
        (void) result;
    }
    timer_stop();
    return timer_read();
}
