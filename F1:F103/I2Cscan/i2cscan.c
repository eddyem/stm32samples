/*
 * This file is part of the I2Cscan project.
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

#include "i2cscan.h"
#include "i2c.h"
//#include "usb.h"

uint8_t scanmode = 0;
static uint8_t i2caddr  = I2C_ADDREND; // not active

uint8_t getI2Caddr(){
    return i2caddr;
}

void i2c_init_scan_mode(){
    i2caddr = 0;
    scanmode = 1;
}

// return 1 if next addr is active & return in as `addr`
// if addresses are over, return 1 and set addr to I2C_NOADDR
// if scan mode inactive, return 0 and set addr to I2C_NOADDR
int i2c_scan_next_addr(uint8_t *addr){
    *addr = i2caddr;
    if(!scanmode || i2caddr == I2C_ADDREND){
        *addr = I2C_ADDREND;
        scanmode = 0;
        return 0;
    }
    i2c_set_addr7(i2caddr++);
    if(I2C_OK != i2c_7bit_send(NULL, 0)) return 0;
    return 1;
}

int read_reg(uint8_t reg, uint8_t *val, uint8_t N){
    if(N == 0){
        if(I2C_OK != i2c_7bit_send_onebyte(reg, 0)) return 0;
        return 1;
    }
    if(I2C_OK != i2c_7bit_send_onebyte(reg, 0)) return 0;
    if(I2C_OK != i2c_7bit_receive(val, N)) return 0;
    return 1;
}

int write_reg(uint8_t reg, uint8_t val){
    uint8_t d[2] = {reg, val};
    if(I2C_OK != i2c_7bit_send(d, 2)) return 0;
    return 1;
}
