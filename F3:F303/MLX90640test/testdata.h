/*
 * This file is part of the mlxtest project.
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

// data from MLX documentation for algorithms testing
// this file should be included only once!

#ifdef _TESTDATA_H__
#error "Don't include this file several times!"
#endif
#define _TESTDATA_H__

#include "mlx90640.h"

static const uint16_t EEPROM[MLX_DMA_MAXLEN] = {
    #include "eeprom.csv"
};

static const int16_t DataFrame[2][MLX_DMA_MAXLEN] = {
    {
    #include "frame0.csv"
    },
    {
    #include "frame1.csv"
    }
};

static const MLX90640_params extracted_parameters = {
     .kVdd =	-3200
    ,.vdd25 =	-12544
    ,.KvPTAT =	0.002197
    ,.KtPTAT =	42.625000
    ,.vPTAT25 =	12196
    ,.alphaPTAT =	9.0
    ,.gainEE =	5580
    ,.tgc =	0.000000
    ,.cpKv =	0.3750
    ,.cpKta =	0.004272
//    ,.calibrationModeEE =	128
    ,.KsTa =	-0.002441
    ,.CT = {0., 300., 500.}
    ,.KsTo = {-0.0002, -0.0002, -0.0002, -0.0002}
    ,.alphacorr = {}
    ,.alpha = {
        #include "alpha.csv"
    }
    ,.offset = {
        #include "offset.csv"
    }
    ,.kta = {
        #include "kta.csv"
    }
    ,.kv = {0.4375, 0.3750, 0.3750, 0.3750}
    ,.cpAlpha = {0.0000000028812792152, 0.0000000029037892091}
    ,.resolEE =	2
    ,.cpOffset = {-69, -65}
    ,.outliers = {0}
};

static const fp_t ToFrame[2][MLX_PIXNO] = {
    {
#include "to_frame0.csv"
    },
    {
#include "to_frame1.csv"
    }
};

