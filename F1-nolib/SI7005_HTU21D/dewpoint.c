/*
 * This file is part of the si7005 project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "dewpoint.h"
#include "usb.h"
#include "proto.h"

#define NKNOTS  (32)
const uint16_t X[NKNOTS] = {1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 19, 22, 29, 37, 47, 59, 72, 87, 104,
                            123, 145, 171, 200, 232, 267, 348, 443, 555, 684, 832};
const float Y[NKNOTS] = {-6.90776, -6.21461, -5.80914, -5.52146, -5.29832, -5.116, -4.96185, -4.82831,
                         -4.60517, -4.42285, -4.2687, -4.13517, -3.96332, -3.81671, -3.54046, -3.29684,
                         -3.05761, -2.83022, -2.63109, -2.44185, -2.26336, -2.09557, -1.93102, -1.76609,
                         -1.60944, -1.46102, -1.32051, -1.05555, -0.814186, -0.588787, -0.379797, -0.183923};
const float K[NKNOTS] = {0.693147, 0.405465, 0.287682, 0.223144, 0.182322, 0.154151, 0.133531, 0.111572,
                         0.0911608, 0.0770753, 0.0667657, 0.0572834, 0.0488678, 0.0394648, 0.0304528,
                         0.023923, 0.0189492, 0.0153176, 0.0126161, 0.010499, 0.00883123, 0.00747952,
                         0.00634345, 0.00540186, 0.00463813, 0.00401461, 0.00327103, 0.00254071,
                         0.00201249, 0.00162008, 0.00132348, 0.00109478};

float ln(uint16_t RH){
    // find interval
    int idx = (NKNOTS)/2, left = 0, right = NKNOTS-1; // left, right, middle
    while(idx > left && idx < right){
        uint16_t midval = X[idx];
        if(RH < midval){
            if(idx == 0) break;
            if(RH > X[idx-1]){ // found
                --idx;
                break;
            }
            right = idx - 1;
            idx = (left + right)/2;
        }else{
            if(RH < X[idx+1]) break;  // found
            left = idx + 1;
            idx = (left + right)/2;
        }
    }
    if(idx < 0) idx = 0;
    else if(idx > NKNOTS-1) idx = NKNOTS - 1;
    //USB_send("idx="); USB_send(u2str(idx)); USB_send("\n");
    float l = Y[idx] + K[idx]*(RH - X[idx]);
#undef NKNOTS
    return l;
}

/**
 * @brief dewpoint calculate dew point (*10degrC)
 * @param T - temperature (*10degrC)
 * @param H - rel. humidity (*10%)
 * @return dewpoint
 */
int32_t dewpoint(int32_t T, uint32_t H){
    float gamma = 17.27 * T / (2377 + T);
    gamma += ln(H);
    float d = 2377 * gamma / (17.27 - gamma);
    return (int32_t)d;
}
