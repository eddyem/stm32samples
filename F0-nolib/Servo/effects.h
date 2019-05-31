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

#pragma once
#ifndef EFFECTS_H__
#define EFFECTS_H__

#include "stm32f0.h"

typedef enum{
    EFF_NONE,
    EFF_WIPE,
    EFF_MADWIPE,
    EFF_PENDULUM,
    EFF_SMPENDULUM,
    EFF_DMASMALL,
    EFF_DMAMED,
    EFF_DMABIG,
    EFF_DMATEST,
    EFF_DMASTAR
} effect_t;

extern uint8_t dma_eff;
void proc_effect();
effect_t set_effect(int n, effect_t eff);

#endif // EFFECTS_H__
