/*
 * This file is part of the as3935 project.
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

#include "as3935.h"
#include "spi.h"

#define MODE_READ   (1 << 6)
#define MODE_WRITE  (0)
#define MODE_MASK   (0x3f)

extern uint32_t Tms;

// read one register
static int as3935_read(uint8_t reg, uint8_t *data){
    uint8_t word[2];
    word[0] = MODE_READ | (reg & MODE_MASK);
    word[1] = 0;
    if(0 == SPI_transmit(word, 2)) return FALSE;
    if(data) *data = word[1];
    return TRUE;
}

static int as3935_write(uint8_t reg, uint8_t data){
    uint8_t word[2];
    word[0] = MODE_WRITE | (reg & MODE_MASK);
    word[1] = data;
    if(0 == SPI_transmit(word, 2)) return FALSE;
    return TRUE;
}

// display on IRQ: nothing (0), TRCO (1), SRCO (2) or LCO (3)
int as3935_displco(uint8_t n){
    if(n > 3) return FALSE;
    t_tun_disp t;
    if(!as3935_read(TUN_DISP, &t.u8)) return FALSE; // we need to save old `tun_cap` value
    t.DISP_LCO = t.DISP_SRCO = t.DISP_TRCO = 0;
    switch(n){
        case 1:
            t.DISP_TRCO = 1;
        break;
        case 2:
            t.DISP_SRCO = 1;
        break;
        case 3:
            t.DISP_LCO = 1;
        break;
        default:
        break;
    }
    return as3935_write(TUN_DISP, t.u8);
}

// tune LCO: change capasitor value
int as3935_tuncap(uint8_t n){
    if(n > 0xf) return FALSE;
    t_tun_disp t;
    if(!as3935_read(TUN_DISP, &t.u8)) return FALSE;
    t.TUN_CAP = n;
    return as3935_write(TUN_DISP, t.u8);
}

// set gain
int as3935_gain(uint8_t n){
    if(n > 0x1f) return FALSE;
    t_afe_gain g;
    if(!as3935_read(AFE_GAIN, &g.u8)) return FALSE;
    g.AFE_GB = n;
    return as3935_write(AFE_GAIN, g.u8);
}

// starting calibration
int as3935_calib_rco(){
    t_tun_disp t;
    if(!as3935_read(TUN_DISP, &t.u8)) return FALSE;
    if(!as3935_write(CALIB_RCO, DIRECT_COMMAND)) return FALSE;
    t.DISP_LCO = t.DISP_TRCO = 0;
    t.DISP_SRCO = 1;
    if(!as3935_write(TUN_DISP, t.u8)) return FALSE;
    uint32_t Tstart = Tms;
    while(Tms - Tstart < 5) IWDG->KR = IWDG_REFRESH; // sleep for 5ms
    t.DISP_SRCO = 0;
    if(!as3935_write(TUN_DISP, t.u8)) return FALSE;
    t_calib srco, trco;
    if(!as3935_read(CALIB_TRCO, &trco.u8)) return FALSE;
    if(!as3935_read(CALIB_SRCO, &srco.u8)) return FALSE;
    if(!srco.CALIB_DONE || !trco.CALIB_DONE) return FALSE;
    return TRUE;
}

// wakeup - call this function after first run
int as3935_wakeup(){
    t_afe_gain g;
    if(!as3935_read(AFE_GAIN, &g.u8)) return FALSE;
    g.PWD = 0;
    if(!as3935_write(AFE_GAIN, g.u8)) return FALSE;
    return as3935_calib_rco();
}

// set amplifier gain
int as3935_set_gain(uint8_t g){
    if(g > 0x1f) return FALSE;
    t_afe_gain a = {0};
    a.AFE_GB = g;
    return as3935_write(AFE_GAIN, a.u8);
}

// watchdog threshold
int as3935_wdthres(uint8_t t){
    if(t > 0x0f) return FALSE;
    t_threshold thres;
    if(!as3935_read(THRESHOLD, &thres.u8)) return FALSE;
    thres.WDTH = t;
    return as3935_write(THRESHOLD, thres.u8);
}

// noice floor level
int as3935_nflev(uint8_t l){
    if(l > 7) return FALSE;
    t_threshold thres;
    if(!as3935_read(THRESHOLD, &thres.u8)) return FALSE;
    thres.NF_LEV = l;
    return as3935_write(THRESHOLD, thres.u8);
}

// spike rejection
int as3935_srej(uint8_t s){
    if(s > 0xf) return FALSE;
    t_lightning_reg lr;
    if(!as3935_read(LIGHTNING_REG, &lr.u8)) return FALSE;
    lr.SREJ = s;
    return as3935_write(LIGHTNING_REG, lr.u8);
}

// minimal lighting number
int as3935_minnumlig(uint8_t n){
    if(n > 3) return FALSE;
    t_lightning_reg lr;
    if(!as3935_read(LIGHTNING_REG, &lr.u8)) return FALSE;
    lr.MIN_NUM_LIG = n;
    return as3935_write(LIGHTNING_REG, lr.u8);
}

// clear amount of lightnings for last 15 min
int as3935_clearstat(){
    t_lightning_reg lr;
    if(!as3935_read(LIGHTNING_REG, &lr.u8)) return FALSE;
    lr.CL_STAT = 1;
    return as3935_write(LIGHTNING_REG, lr.u8);
}

// get interrupt code
int as3935_intcode(uint8_t *code){
    if(!code) return FALSE;
    t_int_mask_ant i;
    if(!as3935_read(INT_MASK_ANT, &i.u8)) return FALSE;
    *code = i.INT;
    return TRUE;
}

// should interrupt react on disturbers?
int as3935_mask_disturber(uint8_t m){
    if(m > 1) return FALSE;
    t_int_mask_ant i;
    if(!as3935_read(INT_MASK_ANT, &i.u8)) return FALSE;
    i.MASK_DIST = m;
    return as3935_write(INT_MASK_ANT, i.u8);
}

// set Fdiv of antenna LCO
int as3935_lco_fdiv(uint8_t d){
    if(d > 3) return FALSE;
    t_int_mask_ant i;
    if(!as3935_read(INT_MASK_ANT, &i.u8)) return FALSE;
    i.LCO_FDIV = d;
    return as3935_write(INT_MASK_ANT, i.u8);
}

// calculate last lightning energy
int as3935_energy(uint32_t *E){
    if(!E) return FALSE;
    uint8_t u; uint32_t energy;
    t_s_lig_mm mm;
    if(!as3935_read(S_LIG_MM, &mm.u8)) return FALSE;
    energy = mm.S_LIG_MM << 8;
    if(!as3935_read(S_LIG_M, &u)) return FALSE;
    energy |= u;
    energy <<= 8;
    if(!as3935_read(S_LIG_L, &u)) return FALSE;
    energy |= u;
    *E = energy;
    return TRUE;
}

// get distance
int as3935_distance(uint8_t *d){
    if(!d) return FALSE;
    t_distance dist;
    if(!as3935_read(DISTANCE, &dist.u8)) return FALSE;
    *d = dist.DISTANCE;
    return TRUE;
}

// reset to factory settings
int as3935_resetdef(){
    return as3935_write(PRESET_DEFAULT, DIRECT_COMMAND);
}
