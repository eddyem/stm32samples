/*
 * This file is part of the Servo project.
 * Copyright 2019 Edward Emelianov <eddy@sao.ru>.
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

#include "effects.h"
#include "hardware.h"
#include "usart.h"

uint8_t dma_eff = 0;

static effect_t current_ef[3] = {EFF_NONE, EFF_NONE, EFF_NONE};
static int8_t cntr[3] = {0,0,0}, dir[3] = {1,1,1};
#define SPD_STP   (25)

static void eff_madwipe(int n){
    static uint32_t speed[3] = {SPD_STP, SPD_STP, SPD_STP};
    if(onposition(n)){ // move back
        if((speed[n]+=SPD_STP) > SG90_STEP) speed[n] = SPD_STP;
        int val = 0;
        if(getPWM(n) < SG90_MIDPULSE) val = 1;
        setPWM(n, val, speed[n]);
    }
}

static void eff_wipe(int n){
    if(onposition(n)){ // move back
        int val = 0;
        if(getPWM(n) < SG90_MIDPULSE) val = 1;
        if(++cntr[n] < 4){ // stay a little in outermost positions
            setPWM(n, getPWM(n), SG90_STEP/2);
        }else{
            cntr[n] = 0;
            setPWM(n, val, SG90_STEP/2);
        }
    }
}

static void eff_pendulum(int n){
    const uint16_t steps[41] = {0, 10, 21, 33, 47, 62, 79, 97, 117, 140, 165, 193, 224, 258, 295, 337, 383, 434,
                             490, 552, 621, 697, 766, 828, 884, 935, 981, 1023, 1060, 1094, 1125, 1153, 1178,
                             1201, 1221, 1239, 1256, 1271, 1285, 1297, 1308};
    if(onposition(n)){
        setPWM(n, SG90_MINPULSE + steps[cntr[n]], SG90_STEP);
        cntr[n] += dir[n];
        if(cntr[n] == -1){ // min position
            dir[n] = 1;
            cntr[n] = 0; // repeat zero position one time
        }else if(cntr[n] == 41){ // max position
            dir[n] = -1;
            cntr[n] = 40; // and this position needs to repeat too
        }
    }
}

static void eff_pendsm(int n){
    const uint16_t steps[19] = {0, 6, 10, 15, 22, 30, 40, 52, 66, 82, 101, 123, 148, 177, 210, 247, 289, 336, 389};
    if(onposition(n)){
        setPWM(n, SG90_MINPULSE + steps[cntr[n]], SG90_STEP);
        cntr[n] += dir[n];
        if(cntr[n] == -1){ // min position
            dir[n] = 1;
            cntr[n] = 1;
        }else if(cntr[n] == 19){ // max position
            dir[n] = -1;
            cntr[n] = 18;
        }
    }
}

// buffers for different DMA effects, by pairs: 1st number is CCR1, 2nd is CCR2
static const uint16_t dmabufsmall[] = { 1400,800,1400,800,
                                        1400,1000,1400,1000,
                                        1600,1000,1600,1000,
                                        1600,800,1600,800,};

static const uint16_t dmabufmed[] = {   1400,800,1400,800,1400,800,1400,800,
                                        1400,1000,1400,1000,1400,1000,1400,1000,
                                        1600,1000,1600,1000,1600,1000,1600,1000,
                                        1600,800,1600,800,1600,800,1600,800};

static const uint16_t dmabufbig[] = {   1400,800,1400,800,1400,800,1400,800,1400,800,1400,800,
                                        1400,1000,1400,1000,1400,1000,1400,1000,1400,1000,1400,1000,
                                        1600,1000,1600,1000,1600,1000,1600,1000,1600,1000,1600,1000,
                                        1600,800,1600,800,1600,800,1600,800,1600,800,1600,800};

static const uint16_t dmabuftest[] = {  1400,800,1400,840,1400,880,1400,920,1400,960,1400,1000,
                                        1400,1000,1440,1000,1480,1000,1520,1000,1560,1000,1600,1000,
                                        1600,1000,1600,960,1600,920,1600,880,1600,840,1600,800,
                                        1600,800,1560,800,1520,800,1480,800,1440,800,1400,800};

static const uint16_t dmabufstar[] = {  1300,930,1300,930,1300,930,1300,930,1300,930,1300,930,
                                        1500,930,1500,930,1500,930,1500,930,1500,930,1500,930,
                                        1330,800,1330,800,1330,800,1330,800,1330,800,1330,800,
                                        1400,1000,1400,1000,1400,1000,1400,1000,1400,1000,1400,1000,
                                        1470,800,1470,800,1470,800,1470,800,1470,800,1470,800};

static void DMA_eff(const void* buff, uint8_t len){
    DMA1_Channel3->CMAR = (uint32_t)(buff);
    DMA1_Channel3->CNDTR = len;
    DMA1_Channel3->CCR |= DMA_CCR_EN;
    TIM3->DIER |= TIM_DIER_UDE;
}

void proc_effect(){
    for(int i = 0; i < 3; ++i){
        switch(current_ef[i]){
            case EFF_WIPE:
                eff_wipe(i);
            break;
            case EFF_MADWIPE:
                eff_madwipe(i);
            break;
            case EFF_PENDULUM:
                eff_pendulum(i);
            break;
            case EFF_SMPENDULUM:
                eff_pendsm(i);
            break;
            case EFF_NONE:
            default:
            break;
        }
    }
}

effect_t set_effect(int n, effect_t eff){
    if(n < 0 || n > 2) return EFF_NONE;
    cntr[n] = 0;
    dir[n] = 1;
    if(dma_eff){
        TIM3->DIER &= ~TIM_DIER_UDE; // turn off DMA requests from UE
        DMA1_Channel3->CCR &= ~DMA_CCR_EN; // turn off DMA if current was with it
        dma_eff = 0;
        TIM3->CCR1 = SG90_MIDPULSE;
        TIM3->CCR2 = SG90_MIDPULSE;
    }
    switch(eff){
        case EFF_DMASMALL:
            DMA_eff(dmabufsmall, sizeof(dmabufsmall)/sizeof(uint16_t));
            dma_eff = 1;
        break;
        case EFF_DMAMED:
            DMA_eff(dmabufmed, sizeof(dmabufmed)/sizeof(uint16_t));
            dma_eff = 1;
        break;
        case EFF_DMABIG:
            DMA_eff(dmabufbig, sizeof(dmabufbig)/sizeof(uint16_t));
            dma_eff = 1;
        break;
        case EFF_DMATEST:
            DMA_eff(dmabuftest, sizeof(dmabuftest)/sizeof(uint16_t));
            dma_eff = 1;
        break;
        case EFF_DMASTAR:
            DMA_eff(dmabufstar, sizeof(dmabufstar)/sizeof(uint16_t));
            dma_eff = 1;
        break;
        default:
        break;
    }
    if(dma_eff){
        current_ef[0] = current_ef[1] = eff;
    }else current_ef[n] = eff;
    return eff;
}
