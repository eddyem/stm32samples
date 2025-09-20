/*
 * This file is part of the mlx90640 project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <string.h>

#include "i2c.h"
#include "mlxproc.h"
#include "mlx90640_regs.h"

// current state and state before `stop` called
static mlx_state_t MLX_state = MLX_NOTINIT, MLX_oldstate = MLX_NOTINIT;
static MLX90640_params p;
static int parsrdy = 0;
static fp_t *ready_image = NULL; // will be pointer to `mlx_image` after both subpages process
static uint8_t MLX_address = 0x33 << 1;
static int errctr = 0; // errors counter - cleared by mlx_continue

// get current state
mlx_state_t mlx_state(){ return MLX_state; }
// set address
int mlx_setaddr(uint8_t addr){
    if(addr > 0x7f) return 0;
    MLX_address = addr << 1;
    return 1;
}
// temporary stop
void mlx_stop(){
    MLX_oldstate = MLX_state;
    MLX_state = MLX_RELAX;
}
// continue processing
void mlx_continue(){
    errctr = 0;
    switch(MLX_oldstate){
        case MLX_WAITSUBPAGE:
        case MLX_READSUBPAGE:
            MLX_state = MLX_WAITSUBPAGE;
        break;
        //case MLX_NOTINIT:
        //case MLX_WAITPARAMS:
        default:
            MLX_state = MLX_NOTINIT;
        break;
    }
}

void mlx_process(){
    //static int subpageno = 0; // wait for given subpage
    switch(MLX_state){
        case MLX_NOTINIT: // start reading parameters
            if(i2c_read_reg16(MLX_address, REG_CALIDATA, MLX_DMA_MAXLEN, 1))
                MLX_state = MLX_WAITPARAMS;
            else ++errctr;
        break;
        case MLX_WAITPARAMS: // check DMA ends and calculate parameters
            if(i2c_dma_haderr()) MLX_state = MLX_NOTINIT;
            else{
                uint16_t len, *buf = i2c_dma_getbuf(&len);
                if(buf){
                    if(len != MLX_DMA_MAXLEN) MLX_state = MLX_NOTINIT;
                    else if(get_parameters(buf, &p)){
                        MLX_state = MLX_WAITSUBPAGE; // fine! we could wait subpage
                        parsrdy = 1;
                    }
                }
            }
        break;
        case MLX_WAITSUBPAGE: // wait for subpage N ready
            ;
        break;
        case MLX_READSUBPAGE: // wait ends of DMA read and calculate subpage
            ;
        break;
        default:
            return;
    }
    if(errctr > MLX_MAX_ERRORS) mlx_stop();
}

// get parameters - memcpy to user's
int mlx_getparams(MLX90640_params *pars){
    if(!pars || !parsrdy) return 0;
    memcpy(pars, &p, sizeof(p));
    return 1;
}

fp_t *mlx_getimage(){
    return ready_image;
}
