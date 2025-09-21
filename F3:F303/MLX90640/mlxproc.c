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
//#include "usb_dev.h"
//#include "strfunc.h"

extern volatile uint32_t Tms;

// current state and state before `stop` called
static mlx_state_t MLX_state = MLX_NOTINIT, MLX_oldstate = MLX_NOTINIT;
static MLX90640_params p;
static int parsrdy = 0;
static fp_t *ready_image = NULL; // will be pointer to `mlx_image` after both subpages process
static uint8_t MLX_address = 0x33 << 1;
static int errctr = 0; // errors counter - cleared by mlx_continue
static uint32_t Tlastimage = 0;
static uint8_t resolution = 2; // default: 18bit

// get current state
mlx_state_t mlx_state(){ return MLX_state; }
// set address
int mlx_setaddr(uint8_t addr){
    if(addr > 0x7f) return 0;
    MLX_address = addr << 1;
    Tlastimage = Tms; // refresh counter for autoreset I2C in case of error
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
    static int subpageno = 0; // wait for given subpage
    // static uint32_t Tlast = 0;
    switch(MLX_state){
        case MLX_NOTINIT: // start reading parameters
            if(i2c_read_reg16(MLX_address, REG_CALIDATA, MLX_DMA_MAXLEN, 1)){
                errctr = 0;
                MLX_state = MLX_WAITPARAMS;
            }else ++errctr;
        break;
        case MLX_WAITPARAMS: // check DMA ends and calculate parameters
            if(i2c_dma_haderr()) MLX_state = MLX_NOTINIT;
            else{
                uint16_t len, *buf = i2c_dma_getbuf(&len);
                if(buf){
                    if(len != MLX_DMA_MAXLEN) MLX_state = MLX_NOTINIT;
                    else if(get_parameters(buf, &p)){
                        errctr = 0;
                        MLX_state = MLX_WAITSUBPAGE; // fine! we could wait subpage
                        parsrdy = 1;
                    }
                }
            }
        break;
        case MLX_WAITSUBPAGE: // wait for subpage N ready
            {uint16_t *got = i2c_read_reg16(MLX_address, REG_STATUS, 1, 0);
            if(got && *got & REG_STATUS_NEWDATA){
                if(subpageno == (*got & REG_STATUS_SPNO)){
                    if(i2c_read_reg16(MLX_address, REG_IMAGEDATA, MLX_DMA_MAXLEN, 1)){
                        errctr = 0;
                        MLX_state = MLX_READSUBPAGE;
                        //U("spstart="); USND(u2str(Tms - Tlast));
                    }else ++errctr;
                }
            }}
        break;
        case MLX_READSUBPAGE: // wait ends of DMA read and calculate subpage
            if(i2c_dma_haderr()) MLX_state = MLX_NOTINIT;
            else{
                uint16_t len, *buf = i2c_dma_getbuf(&len);
                if(buf){
                    //U("spread="); USND(u2str(Tms - Tlast));
                    if(len != MLX_DMA_MAXLEN) MLX_state = MLX_WAITSUBPAGE;
                    else if((ready_image = process_subpage(&p, (int16_t*)buf, subpageno, 2))){
                        errctr = 0;
                        MLX_state = MLX_WAITSUBPAGE; // fine! we could wait subpage
                        //U("spgot="); USND(u2str(Tms - Tlast));
                        if(subpageno){ Tlastimage = Tms;
                            /*U("imgot="); USND(u2str(Tms - Tlast)); Tlast = Tms; */
                        }
                        subpageno = !subpageno;
                    }
                }
            }
        break;
        default:
            return;
    }
    if(MLX_state != MLX_RELAX && Tms - Tlastimage > MLX_I2CERR_TMOUT){ i2c_setup(i2c_curspeed); Tlastimage = Tms; }
    if(errctr > MLX_MAX_ERRORS) mlx_stop();
}

// get parameters - memcpy to user's
int mlx_getparams(MLX90640_params *pars){
    if(!pars || !parsrdy) return 0;
    memcpy(pars, &p, sizeof(p));
    return 1;
}

fp_t *mlx_getimage(uint32_t *Tgot){
    if(Tgot) *Tgot = Tlastimage;
    return ready_image;
}

uint8_t mlx_getresolution(){
    return resolution;
}

int mlx_sethwaddr(uint8_t addr){
    if(addr > 0x7f) return 0;
    uint16_t data[2], *ptr;
    if(!(ptr = i2c_read_reg16(MLX_address, REG_MLXADDR, 1, 0))) return 0;
    data[0] = REG_MLXADDR;
    data[1] = (*ptr & ~REG_MLXADDR_MASK) | addr;
    if(!i2c_write(MLX_address, data, 2)) return 0;
    return 1;
}

int mlx_setresolution(uint8_t newresol){
    if(newresol > 3) return 0;
    uint16_t data[2], *ptr;
    if(!(ptr = i2c_read_reg16(MLX_address, REG_CONTROL, 1, 0))) return 0;
    data[0] = REG_CONTROL;
    data[1] = (*ptr & ~REG_CONTROL_RESMASK) | (newresol << 10);
    if(!i2c_write(MLX_address, data, 2)) return 0;
    resolution = newresol;
    return 1;
}
