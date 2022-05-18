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

extern uint32_t Tms;

static const char* _states[M_STATES_AMOUNT] = {
    [M_ERROR]       = "error",
    [M_RELAX]       = "do nothing",
    [M_FIRSTSTART]  = "first start",
    [M_READCONF]    = "read config",
    [M_STARTIMA]    = "start image",
    [M_PROCESS]     = "process subframe",
    [M_READOUT]     = "read subpage data",
    [M_POWERON]     = "wait after power on",
    [M_POWEROFF1]   = "turn power off",
    [M_POWEROFF]    = "wait without power",
};

// dump floating point array 24x32
static void dumpfarr(float *arr){
    for(int row = 0; row < 24; ++row){
        for(int col = 0; col < 32; ++col){
            float2str(*arr++, 2); bufputchar(' ');
        }
        newline();
    }
}

static void dumpparams(){
    int16_t *pi16;
    SEND("\nkVdd="); printi(params.kVdd);
    SEND("\nvdd25="); printi(params.vdd25);
    SEND("\nKvPTAT="); float2str(params.KvPTAT, 4);
    SEND("\nKtPTAT="); float2str(params.KtPTAT, 4);
    SEND("\nvPTAT25="); printi(params.vPTAT25);
    SEND("\nalphaPTAT="); float2str(params.alphaPTAT, 2);
    SEND("\ngainEE="); printi(params.gainEE);
    SEND("\nPixel offset parameters:\n");
    pi16 = params.offset;
    for(int row = 0; row < 24; ++row){
        for(int col = 0; col < 32; ++col){
            printi(*pi16++); bufputchar(' ');
        }
        newline();
    }
    SEND("K_talpha:\n");
    dumpfarr(params.kta);
    SEND("Kv: ");
    for(int i = 0; i < 4; ++i){
        float2str(params.kv[i], 2); bufputchar(' ');
    }
    SEND("\ncpOffset=");
    printi(params.cpOffset[0]); SEND(", "); printi(params.cpOffset[1]);
    SEND("\ncpKta="); float2str(params.cpKta, 2);
    SEND("\ncpKv="); float2str(params.cpKv, 2);
    SEND("\ntgc="); float2str(params.tgc, 2);
    SEND("\ncpALpha="); float2str(params.cpAlpha[0], 2);
    SEND(", "); float2str(params.cpAlpha[1], 2);
    SEND("\nKsTa="); float2str(params.KsTa, 2);
    SEND("\nAlpha:\n");
    dumpfarr(params.alpha);
    SEND("\nCT3="); float2str(params.CT[1], 2);
    SEND("\nCT4="); float2str(params.CT[2], 2);
    for(int i = 0; i < 4; ++i){
        SEND("\nKsTo"); bufputchar('0'+i); bufputchar('=');
        float2str(params.ksTo[i], 2);
        SEND("\nalphacorr"); bufputchar('0'+i); bufputchar('=');
        float2str(params.alphacorr[i], 2);
    }
    NL();
}

const char *parse_cmd(char *buf){
    int32_t Num = 0;
    uint16_t r, d;
    uint16_t data[D16LEN];
    const float pi = 3.1415927f, e = 2.7182818f;
    char *ptr, cmd = *buf++;
    switch(cmd){
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
        case 'E':
        case 'e':
            if(!mlx90640_take_image(cmd == 'e')) return "FAILED";
            else return "OK";
        break;
        case 'f':
            SEND("Float test: ");
            float2str(0.f, 2); addtobuf(", ");
            float2str(pi, 1); addtobuf(", ");
            float2str(-e, 2); addtobuf(", ");
            float2str(-pi, 3); addtobuf(", ");
            float2str(e, 4); addtobuf(", ");
            uint32_t uu = INF | 0x80000000;
            float *f = (float*)&uu;
            float2str(*f, 4); addtobuf(", ");
            uu = NAN;
            f = (float*)&uu;
            float2str(*f, 4);
            NL();
            return NULL;
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
        case 'M':
            SEND("MLX state: "); SEND(_states[mlx_state]);
            SEND("\npower="); printu(MLXPOW_VAL()); NL();
            return NULL;
        break;
        case 'O':
            mlx90640_restart();
            return "Power off/on";
        break;
        case 'P':
            dumpparams();
            return NULL;
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
        case 'T':
            SEND("Tms="); printu(Tms); NL();
            return NULL;
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
            "'Ee' - expose image: E - full, e - simple\n"
            "'f' - test float printf (0.00, 3.1, -2.72, -3.142, 2.7183, -INF, NAN)\n"
            "'g reg N' - read N (<256) registers starting from `reg`\n"
            "'I' - restart I2C\n"
            "'M' - MLX state\n"
            "'O' - turn On or restart MLX sensor\n"
            "'P' - dump params\n"
            "'r reg' - read `reg`\n"
            "'R' - software reset\n"
            "'T' - get Tms\n"
            "'w reg dword' - write `dword` to `reg`\n"
            "'W d0 d1 ...' - write N (<256) 16-bit words directly to I2C\n"
            );
            NL();
            return NULL;
        break;
    }
    return NULL;
}
