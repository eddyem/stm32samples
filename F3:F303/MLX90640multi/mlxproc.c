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

//#define DEBUGPROC

#ifdef DEBUGPROC
#include "usb_dev.h"
#include "strfunc.h"
#define D(x)        U(x)
#define DN(x)       USND(x)
#define DB(x)       USB_putbute(x)
#else
#define D(x)
#define DN(x)
#define DB(x)
#endif

extern volatile uint32_t Tms;

// current state and state before `stop` called
static mlx_state_t MLX_state = MLX_RELAX, MLX_oldstate = MLX_RELAX;
static int errctr = 0; // errors counter - cleared by mlx_continue
static uint32_t Tlastimage[N_SESORS] = {0};

// subpages and configs of all sensors
// 8320 bytes:
static int16_t imdata[N_SESORS][REG_IMAGEDATA_LEN];
// 8340 bytes:
static uint16_t confdata[N_SESORS][MLX_DMA_MAXLEN];
static uint8_t sens_addresses[N_SESORS] = {0x10<<1, 0x11<<1, 0x12<<1, 0x13<<1, 0x14<<1}; // addresses of all sensors (if 0 - omit this one)
static uint8_t sensaddr[N_SESORS];

// get compile-time size: (gcc shows it in error message)
//char (*__kaboom)[sizeof( confdata )] = 1;

// return `sensaddr`
uint8_t *mlx_activeids(){return sensaddr;}

static int sensno = -1;

// get current state
mlx_state_t mlx_state(){ return MLX_state; }
// set address
int mlx_setaddr(int n, uint8_t addr){
    if(n < 0 || n > N_SESORS) return 0;
    if(addr > 0x7f) return 0;
    sens_addresses[n] = addr << 1;
    Tlastimage[n] = Tms; // refresh counter for autoreset I2C in case of error
    return 1;
}
// pause state machine and stop
void mlx_pause(){
    MLX_oldstate = MLX_state;
    MLX_state = MLX_RELAX;
}
// TODO: add here power management
void mlx_stop(){
    MLX_oldstate = MLX_NOTINIT;
    MLX_state = MLX_RELAX;
}

// continue processing
// TODO: add here power management
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
            i2c_setup(i2c_curspeed); // restart I2C (what if there was errors?)
            memcpy(sensaddr, sens_addresses, sizeof(sens_addresses));
            MLX_state = MLX_NOTINIT;
            sensno = -1;
        break;
    }
}

static int nextsensno(int s){
    if(mlx_nactive() == 0){
        mlx_stop();
        return -1;
    }
    int next = s + 1;
    for(; next < N_SESORS; ++next) if(sensaddr[next]) break;
    if(next == N_SESORS) return nextsensno(-1); // roll to start
        D(i2str(next)); DB('('); D(i2str(s)); DB(')'); DN(" - new sensor number");
    return next;
}

// count active sensors
int mlx_nactive(){
    int N = 0;
    for(int i = 0; i < N_SESORS; ++i) if(sensaddr[i]) ++N;
    return N;
}

/**
 * @brief mlx_process - main state machine
 * 1. Process conf data for each sensor
 * 2. Start image processing and store subpage1 for each sensor
 */
void mlx_process(){
    static uint32_t TT = 0;
    if(Tms == TT) return;
    TT = Tms;
    //    static uint32_t Tlast = 0;
    static int subpage = 0;
    if(MLX_state == MLX_RELAX) return;
    if(sensno == -1){ // init
        sensno = nextsensno(-1);
        if(-1 == sensno) return; // no sensors found
    }
    switch(MLX_state){
        case MLX_NOTINIT: // start reading parameters
            if(i2c_read_reg16(sensaddr[sensno], REG_CALIDATA, MLX_DMA_MAXLEN, 1)){
                    D(i2str(sensno)); DN(" wait conf");
                errctr = 0;
                MLX_state = MLX_WAITPARAMS;
            }else ++errctr;
        break;
        case MLX_WAITPARAMS: // check DMA ends and calculate parameters
            if(i2c_dma_haderr()){ MLX_state = MLX_NOTINIT; DN("DMA err");}
            else{
                uint16_t len, *buf = i2c_dma_getbuf(&len);
                if(!buf) break;
                DN("READ");
                if(len != MLX_DMA_MAXLEN){ MLX_state = MLX_NOTINIT; break; }
                memcpy(confdata[sensno], buf, MLX_DMA_MAXLEN * sizeof(uint16_t));
                    D(i2str(sensno)); DN(" got conf");
                int next = nextsensno(sensno);
                errctr = 0;
                if(next <= sensno) MLX_state = MLX_WAITSUBPAGE; // all configuration read
                else MLX_state = MLX_NOTINIT; // read next
                sensno = next;
                return;
            }
        break;
        case MLX_WAITSUBPAGE: // wait for subpage 1 ready
            {uint16_t *got = i2c_read_reg16(sensaddr[sensno], REG_STATUS, 1, 0);
            if(got && *got & REG_STATUS_NEWDATA){
                if(subpage == (*got & REG_STATUS_SPNO)){
                    errctr = 0;
                    if(subpage == 0){ // omit zero subpage for each sensor
                        DN("omit 0 -> next sens");
                        int next = nextsensno(sensno);
                        if(next <= sensno){ // all scanned - now wait for page 1
                            subpage = 1;
                            DN("Wait for 1");
                        }
                        sensno = next;
                        break;
                    }
                    D(i2str(sensno)); DN(" - ask for image");
                    if(i2c_read_reg16(sensaddr[sensno], REG_IMAGEDATA, REG_IMAGEDATA_LEN, 1)){
                        errctr = 0;
                        MLX_state = MLX_READSUBPAGE;
                        //    D("spstart"); DB('0'+subpage); DB('='); DN(u2str(Tms - Tlast));
                    }else ++errctr;
                }
            }else ++errctr;
            }
        break;
        case MLX_READSUBPAGE: // wait ends of DMA read and calculate subpage
            if(i2c_dma_haderr()) MLX_state = MLX_WAITSUBPAGE;
            else{
                uint16_t len, *buf = i2c_dma_getbuf(&len);
                if(buf){
                    //    D("spread="); DN(u2str(Tms - Tlast));
                    if(len != REG_IMAGEDATA_LEN){
                        ++errctr;
                    }else{ // fine! we could check next sensor
                        errctr = 0;
                        memcpy(imdata[sensno], buf, REG_IMAGEDATA_LEN * sizeof(int16_t));
                        //  D("spgot="); DN(u2str(Tms - Tlast));
                        Tlastimage[sensno] = Tms;
                        //  D("imgot="); DN(u2str(Tms - Tlast)); Tlast = Tms;
                        int next = nextsensno(sensno);
                        if(next <= sensno){
                            subpage = 0; // roll to start - omit page 0 for all
                            DN("All got -> start from 0");
                        }
                        sensno = next;
                    }
                    MLX_state = MLX_WAITSUBPAGE;
                }
            }
        break;
        default:
            return;
    }
    //if(MLX_state != MLX_RELAX && Tms - Tlastimage[sensno] > MLX_I2CERR_TMOUT){ i2c_setup(i2c_curspeed); Tlastimage[sensno] = Tms; }
    if(errctr > MLX_MAX_ERRORS){
        errctr = 0;
        sensaddr[sensno] = 0; // throw out this value
        sensno = nextsensno(sensno);
    }
}

// recalculate parameters
MLX90640_params *mlx_getparams(int n){
    MLX90640_params *p = get_parameters(confdata[n]);
    return p;
}

uint32_t mlx_lastimT(int n){ return Tlastimage[n]; }

fp_t *mlx_getimage(int n){
    if(n < 0 || n >= N_SESORS || !sensaddr[n]) return NULL;
    MLX90640_params *p = get_parameters(confdata[n]);
    if(!p) return NULL;
    fp_t *ready_image = process_image(imdata[n]);
    if(!ready_image) return NULL;
    return ready_image;
}

// this function can be run only when state machine is paused/stopped!
// WARNING: `MLX_address` is shifted, `addr` - NOT!
int mlx_sethwaddr(uint8_t MLX_address, uint8_t addr){
    if(addr > 0x7f) return 0;
    uint16_t data[2], *ptr;
    if(!(ptr = i2c_read_reg16(MLX_address, REG_MLXADDR, 1, 0))) return 0;
    //D("Old address: "); DN(uhex2str(*ptr));
    data[0] = REG_MLXADDR; data[1] = 0;
    uint16_t oldreg = *ptr;
    if(!i2c_write(MLX_address, data, 2)) return 0; // clear address
    uint32_t Told = Tms;
    while(Tms - Told < 10);
    ptr = i2c_read_reg16(MLX_address, REG_MLXADDR, 1, 0);
    // should be zero
    if(!ptr){
        data[0] = REG_MLXADDR; data[1] = oldreg;
        i2c_write(MLX_address, data, 2); // leave old address
        return 0;
    }
    data[0] = REG_MLXADDR; // i2c_write swaps bytes, so we need init data again
    data[1] = (oldreg & ~REG_MLXADDR_MASK) | addr;
    //D("Write address: "); D(uhex2str(data[0])); D(", "); DN(uhex2str(data[1]));
    if(!i2c_write(MLX_address, data, 2)) return 0;
    while(Tms - Told < 10);
    if(!(ptr = i2c_read_reg16(MLX_address, REG_MLXADDR, 1, 0))) return 0;
    //D("Got address: "); DN(uhex2str(*ptr));
    if((*ptr & REG_MLXADDR_MASK) != addr) return 0;
    return 1;
}

