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

#include <stm32g4.h>
#include <math.h>
#include "cordic.h"

// default CSR settings
#define CORDIC_CSR_DEF  (5 << CORDIC_CSR_PRECISION_Pos)

// enable CORDIC
static void cordic_enable(void){
    RCC->AHB1ENR |= RCC_AHB1ENR_CORDICEN;
    __DSB();
}

// Convert float to Q1.31 in [-1, 1)
static int32_t float_to_q31(float x){
    if(x >= 1.0f) x = 1.0f - 1e-6f;
    if(x <= -1.0f) x = -1.0f + 1e-6f;
    return (int32_t)(x * 2147483648.0f);
}

// Convert Q1.31 to float
static float q31_to_float(int32_t q){
    return (float)q / 2147483648.0f;
}

// Wait CORDIC ready
static void cordic_wait_ready(void){
    while(!(CORDIC->CSR & CORDIC_CSR_RRDY));
}

static void cordic_init(void){
    static bool inited = false;
    if(!inited){
        cordic_enable();
        inited = true;
    }
}

#if 0
// Common function for one arg (sin, cos, sqrt, log)
static float cordic_scalar(uint32_t func_mode, float x, int arg_count){
    cordic_init();
    CORDIC->CSR = (func_mode << CORDIC_CSR_FUNC_Pos) |
                  CORDIC_CSR_INSIZE_1 |            // in Q1.31
                  CORDIC_CSR_OUTSIZE_0;            // out Q1.31
    if(arg_count == 1){
        CORDIC->WDATA = float_to_q31(x);
    } else {
...
    }
    cordic_wait_ready();
    int32_t res = CORDIC->RDATA;
    return q31_to_float(res);
}
#endif

// sin
float cordic_sin(float angle){
    float norm = angle / 3.141592653589793f;
    if(norm > 1.0f) norm = 1.0f;
    if(norm < -1.0f) norm = -1.0f;
    cordic_init();
    CORDIC->CSR = (CORDIC_CSR_FUNC_SIN << CORDIC_CSR_FUNC_Pos) | CORDIC_CSR_DEF;
    CORDIC->WDATA = float_to_q31(norm);
    cordic_wait_ready();
    int32_t res = CORDIC->RDATA; // первое чтение -> sin
    return q31_to_float(res);
}

// cos
float cordic_cos(float angle){
    float norm = angle / 3.141592653589793f;
    if(norm > 1.0f) norm = 1.0f;
    if(norm < -1.0f) norm = -1.0f;
    cordic_init();
    CORDIC->CSR = (CORDIC_CSR_FUNC_COS << CORDIC_CSR_FUNC_Pos) | CORDIC_CSR_DEF;
    CORDIC->WDATA = float_to_q31(norm);
    cordic_wait_ready();
    int32_t res = CORDIC->RDATA; // cos
    return q31_to_float(res);
}

// atan2
float cordic_atan(float val){
    if(val > 1.f || val < -1.f) return NAN;
    cordic_init();
    CORDIC->CSR = (CORDIC_CSR_FUNC_ATAN << CORDIC_CSR_FUNC_Pos) | CORDIC_CSR_DEF;
    CORDIC->WDATA = float_to_q31(val);
    cordic_wait_ready();
    int32_t res = CORDIC->RDATA;
    return q31_to_float(res) * 3.141592653589793f;
}

// sqrt
float cordic_sqrt(float x){
    if(x < 0.0f) x = 0.0f;
    float scale = 1.0f;
    if(x > 1.0f){
        if(x < 100.f){ scale = 10.f; x /= 100.f;}
        else if(x < 1e4f){ scale = 100.f; x /= 1e4f;}
        else if(x < 1e6f){ scale = 1e3f; x /= 1e6f;}
        else if(x < 1e10f){ scale = 1e5f; x /= 1e10f;}
        else return NAN; // number is too big
    }
    cordic_init();
    CORDIC->CSR = (CORDIC_CSR_FUNC_SQRT << CORDIC_CSR_FUNC_Pos) | CORDIC_CSR_DEF;
    CORDIC->WDATA = float_to_q31(x);
    cordic_wait_ready();
    int32_t res = CORDIC->RDATA;
    return scale * q31_to_float(res);
}

// log
float cordic_log(float x){
    if(x <= 0.0f) x = 0.00001f;
    float add = 0.f;
    if(x > 1.0f){
        if(x < 1000.f){ add = 6.907755279f; x /= 1000.f; }
        else if(x < 1e6f){ add = 13.815510558f; x /= 1e6f; }
        else if(x < 1e12f){ add = 27.63102112f; x /= 1e12f; }
        else return NAN;
    }
    cordic_init();
    CORDIC->CSR = (CORDIC_CSR_FUNC_LOG << CORDIC_CSR_FUNC_Pos) | CORDIC_CSR_DEF;
    CORDIC->WDATA = float_to_q31(x);
    cordic_wait_ready();
    int32_t res = CORDIC->RDATA;
    return q31_to_float(res) * 0.6931471805599453f + add;
}
