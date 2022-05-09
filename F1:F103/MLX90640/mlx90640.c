/*
 * This file is part of the MLX90640 project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "i2c.h"
#include "mlx90640.h"
#include "strfunct.h"

// read register value
int read_reg(uint16_t reg, uint16_t *val){
    uint8_t _2bytes[2];
    _2bytes[0] = reg >> 8; // big endian!
    _2bytes[1] = reg & 0xff;
    if(I2C_OK != i2c_7bit_send(_2bytes, 2, 0)){
        DBG("Can't send address");
        return 0;
    }
    i2c_status s = i2c_7bit_receive_twobytes(_2bytes);
    if(I2C_OK != s){
#ifdef EBUG
        DBG("Can't get info, s=");
        printu(s); NL();
#endif
        return 0;
    }
    *val = _2bytes[0] << 8 | _2bytes[1]; // big endian -> little endian
    return 1;
}

// read N uint16_t values starting from `reg`
// @return N of registers read
int read_data(uint16_t reg, uint16_t *data, int N){
    if(N < 1 ) return 0;
    int i;
    for(i = 0; i < N; ++i){
        if(!read_reg(reg+i, data++)) break;
    }
    return i;
}

// write register value
int write_reg(uint16_t reg, uint16_t val){
    // little endian -> big endian
    uint8_t _4bytes[4];
    _4bytes[0] = reg >> 8;
    _4bytes[1] = reg & 0xff;
    _4bytes[2] = val >> 8;
    _4bytes[3] = val & 0xff;
    if(I2C_OK != i2c_7bit_send(_4bytes, 4, 1)) return 0;
    return 1;
}
