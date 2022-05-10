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
#include "proto.h"
#include "strfunct.h"
#include "usb.h"
#include "version.inc"

#define D16LEN  (256)

const char *parse_cmd(char *buf){
    int32_t Num = 0;
    uint16_t r, d;
    uint16_t data[D16LEN];
    char *ptr;
    switch(*buf++){
        case 'a':
            if(buf != getnum(buf, &Num)){
                if(Num & 0x80) return "Enter 7bit address";
                i2c_set_addr7(Num);
                return "Changed";
            }else return "Wrong address";
        break;
        case 'd':
            if(buf != (ptr = getnum(buf, &Num))){
                r = Num;
                if(ptr != getnum(ptr, &Num)){
                    if(Num < 1) return "N>0";
                    if(!read_data_dma(r, Num)) return("Can't read");
                    else return "OK";
                }else return "Need amount";
            }else return "Need reg";
        break;
        case 'g':
            if(buf != (ptr = getnum(buf, &Num))){
                r = Num;
                if(ptr != getnum(ptr, &Num)){
                    if(Num < 1 || Num > 256) return "N from 0 to 256";
                    d = read_data(r, data, Num);
                    if(d < Num){
                        addtobuf("Got only ");
                        printu(d);
                        addtobuf(" values\n");
                    }
                    for(uint16_t i = 0; i < d; ++i){
                        printuhex(r + i);
                        addtobuf(" ");
                        printuhex(data[i]);
                        newline();
                    }
                    sendbuf();
                    return NULL;
                }else return "Need amount";
            }else return "Need reg";
        break;
        case 'I':
            i2c_setup(TRUE);
            return "I2C restarted";
        break;
        case 'r':
            if(buf != (ptr = getnum(buf, &Num))){
                if(read_reg(Num, &d)){
                    printuhex(d); NL();
                    return NULL;
                }else return "Can't read";
            }else return "Need register address";
        break;
        case 'R':
            USB_sendstr("Soft reset\n");
            NVIC_SystemReset();
        break;
        case 'w':
            if(buf == (ptr = getnum(buf, &Num))) return "Need register";
            r = Num;
            if(ptr == getnum(ptr, &Num)) return "Need data";
            if(write_reg(r, Num)) return "OK";
            else return "Failed";
        break;
        case 'W':
            r = 0;
            while(r < D16LEN){
                if(buf == (ptr = getnum(buf, &Num))) break;
                data[r++] = ((Num & 0xff) << 8) | (Num >> 8);
                buf = ptr + 1;
            }
            if(r == 0) return "Need at least one uint8_t";
            if(I2C_OK == i2c_7bit_send((uint8_t*)data, r*2, 1)) return "Sent\n";
            else return "Error\n";
        break;
        default: // help
            addtobuf(
            "MLX90640 build #" BUILD_NUMBER " @" BUILD_DATE "\n\n"
            "'a addr' - change MLX I2C address to `addr`\n"
            "'d reg N' - read  registers starting from `reg` using DMA\n"
            "'g reg N' - read N (<256) registers starting from `reg`\n"
            "'I' - restart I2C\n"
            "'r reg' - read `reg`\n"
            "'R' - software reset\n"
            "'w reg dword' - write `dword` to `reg`\n"
            "'W d0 d1 ...' - write N (<256) 16-bit words directly to I2C\n"
            );
            NL();
            return NULL;
        break;
    }
    return NULL;
}
