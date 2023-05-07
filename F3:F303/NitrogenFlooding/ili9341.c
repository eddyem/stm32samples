/*
 * This file is part of the nitrogen project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "hardware.h"
#include "ili9341.h"
#include "spi.h"
#ifdef EBUG
#include "strfunc.h"
#endif
#include "usb.h"

/**
 * @brief il9341_readreg - read data from register
 * @param reg - register
 * @param data (i) - data
 * @param N - length of data
 * @return 0 if failed
 */
int ili9341_readreg(uint8_t reg, uint8_t *data, uint32_t N){
    SCRN_Command();
    if(!spi_write(&reg, 1)) return 0;
    if(!spi_waitbsy()) return 0;
    SCRN_Data();
    if(!spi_read(data, N)) return 0;
    if(!spi_waitbsy()) return 0;
    SCRN_Command();
    return 1;
}

/**
 * @brief il9341_writereg - write data to register
 * @param reg - register
 * @param data (o) - data
 * @param N - length of data
 * @return 0 if failed
 */
int ili9341_writereg(uint8_t _U_ reg, const uint8_t _U_ *data, uint32_t _U_ N){
    SCRN_Command();
    if(!spi_write(&reg, 1)) return 0;
    if(!spi_waitbsy()) return 0;
    SCRN_Data();
    if(!spi_write(data, N)) return 0;
    if(!spi_waitbsy()) return 0;
    SCRN_Command();
    return 1;
}
