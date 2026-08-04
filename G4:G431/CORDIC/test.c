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

#include "test.h"
#include "hardware.h"
#include "cordic.h"

// amount of iterations over test
#define N_TESTS     1000

// RNG
static uint32_t rand_state = 123456789;
static uint32_t next_rand(void){
    rand_state = rand_state * 1664525 + 1013904223;
    return rand_state;
}

// fill array with random angles
static void fill_random_sin_cos(float *arr){
    for(int i = 0; i < N_TESTS; ++i){
        // angle from -pi to +pi
        arr[i] = (float)(next_rand() % 62831853) / 10000000.0f - 3.14159265f;
    }
}

static void fill_random_atan(float *arr){
    for(int i = 0; i < N_TESTS; ++i){
        arr[i] = (float)(next_rand() % 2000000) / 1e6f - 1e6f; // [-1,1]
    }
}

static void fill_random_sqrt(float *arr){
    for(int i = 0; i < N_TESTS; ++i){
        arr[i] = (float)(next_rand() % 10000) / 100.0f; // [0,100]
    }
}

static void fill_random_log(float *arr){
    for(int i = 0; i < N_TESTS; ++i){
        arr[i] = (float)(next_rand() % 10000 + 1) / 100.0f; // [0.01, 100]
    }
}

// main test template
static uint32_t run_test(void (*gen)(float*), float (*func)(float)){
    float arr[N_TESTS];
    gen(arr);
    volatile float result = 0.0f; // don't let gcc to optimize this cycle
    timer_start();
    for(int i = 0; i < N_TESTS; ++i){
        result = func(arr[i]);
        (void) result;
    }
    timer_stop();
    return timer_read();
}

// ------------- math.h tests -------------
uint32_t test_math_sin(void){
    return run_test(fill_random_sin_cos, sinf);
}
uint32_t test_math_cos(void){
    return run_test(fill_random_sin_cos, cosf);
}
uint32_t test_math_atan(void){
    return run_test(fill_random_atan, atanf);
}
uint32_t test_math_sqrt(void){
    return run_test(fill_random_sqrt, sqrtf);
}
uint32_t test_math_log(void){
    return run_test(fill_random_log, logf);
}

// ------------- CORDIC tests -------------
uint32_t test_cordic_sin(void){
    return run_test(fill_random_sin_cos, cordic_sin);
}
uint32_t test_cordic_cos(void){
    return run_test(fill_random_sin_cos, cordic_cos);
}
uint32_t test_cordic_atan(void){
    return run_test(fill_random_atan, cordic_atan);
}
uint32_t test_cordic_sqrt(void){
    return run_test(fill_random_sqrt, cordic_sqrt);
}
uint32_t test_cordic_log(void){
    return run_test(fill_random_log, cordic_log);
}
